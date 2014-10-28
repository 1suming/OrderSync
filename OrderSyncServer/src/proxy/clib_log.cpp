// vim: set expandtab tabstop=4 shiftwidth=4 fdm=marker:
// +----------------------------------------------------------------------+
// | Club Library.                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 2005-2006 Tencent Inc. All Rights Reserved.            |
// +----------------------------------------------------------------------+
// | Authors: The Club Dev Team, ISRD, Tencent Inc.                       |
// |          hyperjiang <hyperjiang@tencent.com>                         |
// +----------------------------------------------------------------------+
// $Id$

/**
 * @version 1.0
 * @author  hyperjiang
 * @date    2006/01/09
 * @brief   CLIB log.
 */
#ifndef WIN32

#include "clib_log.h" 


static int createSem(int SemKey,int *pSID,char* sErrMsg)
{
#ifndef SLACKWARE
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    };  
#endif

    union semun arg;
    int iFirst;  //是否第一次创建

    //初始化信号灯
    iFirst=1;
    if ((*pSID = semget(SemKey, 1, 0666 | IPC_CREAT | IPC_EXCL )) <0 )
    {

        if ((*pSID = semget(SemKey, 1, 0 )) == -1 )
        {
            if (sErrMsg)
                sprintf(sErrMsg,"semget error: key=%d,%m",SemKey);
            return -1;
        }

        iFirst=0;
    }       

    if (iFirst)
    {
        arg.val=1;  
        if ( semctl(*pSID, 0, SETVAL, arg ) == -1 )
        {
            if (sErrMsg)
                sprintf(sErrMsg,"semctl error: key=%d,%m",SemKey);
            return -2;
        }

    }

    return 0;
}

static int SemLock(int SID,char* sErrMsg)
{
    struct sembuf sops = {0, -1, SEM_UNDO};

    if (SID==-1)
        return -1;

    if(semop(SID,&sops,1)==-1){
        if (sErrMsg)
            sprintf(sErrMsg,"semop error: sid=%d,%m",SID);
        return(-2);
    }

    return 0;       
}

static int SemUnlock(int SID,char* sErrMsg)
{
    struct sembuf sops = {0, 1, SEM_UNDO};
    size_t nsops = 1;

    if (SID==-1)
        return -1;

    if ( semop(SID, &sops, nsops ) == -1 ){
        if (sErrMsg)
            sprintf(sErrMsg,"semop error: sid=%d,%m",SID);
        return -2;
    }

    return 0;
}


/* {{{ int clib_log_write() */
/**
 * Write Log.
 *
 * @param   as_file       Log filename/directory.
 * @param   as_msg        Log message.
 * @param   ai_len        Message length.
 * @return                0: succ, other: fail.
 */
int clib_log_write( const char *as_file, const char *as_msg, size_t ai_len )
{
    int fd = open( as_file, O_WRONLY|O_APPEND|O_LARGEFILE|O_CREAT, 0666 ); //use local variable to avoid locking
    if( fd > 0 ) {
        write( fd, as_msg, ai_len );
        close( fd );
    } else {
        return(-1);
    } // if
    return(0);
}
/* }}} */


/* {{{ int clib_clog::check_file() */
/**
 * Check if need to change log filename.
 *
 * @param   as_file     Check log filename.
 * @return              0: succ, !=0: fail.
 */
int clib_log::check_file( char *as_file )
{
    int     i = 0;
    struct  stat st_stat;
    char    s_oldfile[CLIB_LOG_SLEN_LONG];
    char    s_newfile[CLIB_LOG_SLEN_LONG];

    //memset( s_oldfile, 0, sizeof(s_oldfile) );
    //memset( s_newfile, 0, sizeof(s_newfile) );

    if ( as_file == NULL || strlen(as_file) <= 0 ) {
        return( CLIB_LOG_ERR_NULL );
    } // if

    if ( mi_maxfile > 0 && mi_maxsize > 0 ) {
        stat( as_file, &st_stat );
        if ( st_stat.st_size > mi_maxsize ) {
            for ( i = mi_maxfile; i > 0; i-- ) {
                if ( i == 1 ) {
                    snprintf( s_newfile, sizeof(s_newfile), "%s.%d", as_file, i );
                    rename( as_file, s_newfile );
                } else {
                    snprintf( s_oldfile, sizeof(s_oldfile), "%s.%d", as_file, i-1 );
                    snprintf( s_newfile, sizeof(s_newfile), "%s.%d", as_file, i );
                    rename( s_oldfile, s_newfile );
                } // if
            } // for
        } // if ( st_stat.st_size > mi_maxsize ) 
    } // if ( mi_maxfile > 0 && mi_maxsize > 0 ) 
    return( CLIB_LOG_OK );
}
/* }}} */


/* {{{ clib_clog::clib_log() */
/**
 * Initial clib_log class.
 *
 * @param   as_file     Log filename.
 * @param   as_tformat  Log timeformat, define in CLIB_LOG_TFORMAT_x.
 * @param   ai_level    Log level, log will write smaller than this level.
 * @param   ai_maxfile  Max log file number (0 mean no change file, maxsize no use).
 * @param   ai_maxsize  Max log file size (Byte, 0 mean unlimited).
 * @param   as_logtype  Log filename type, define in CLIB_LOG_TYPE_x.
 * @param   as_eol      End of line.
 * @param   ai_pathtype Log filename path type (define CLIB_LOG_PATH_xxx).
 */
clib_log::clib_log( const char *as_file,
                    int         as_sem,
                    const char *as_tformat,
                    int         ai_level,
                    int         ai_maxfile,
                    int         ai_maxsize,
                    const char *as_logtype,
                    const char *as_eol,
                    int         ai_pathtype,
                    const char *as_coloring)
{
    memset( ms_file,   0, sizeof(ms_file) );

    if ( as_file != NULL ) {
        if ( ai_pathtype == CLIB_LOG_PATH_RELATIVE ) {
            snprintf( ms_file, sizeof(ms_file), "%s/%s",
                        CLIB_LOG_PATH, as_file );
        } else {
            snprintf( ms_file, sizeof(ms_file), "%s", as_file );
        } // if

        //init semopher
        int fd = open( ms_file, O_WRONLY|O_APPEND|O_CREAT, 0666 ); 
        if (fd > 0)
        {
            close(fd);
        }
        key_t ipc_key = ftok( ms_file, 0 );
        mi_semopen = as_sem;
        if (mi_semopen && ipc_key > 0)
        {
            createSem(ipc_key, &mi_semid, NULL);
        }

    } // if ( as_file != NULL )

    if ( NULL != as_tformat ) {
        snprintf( ms_tformat, sizeof(ms_tformat), "%s", as_tformat );
    } else {
        snprintf( ms_tformat, sizeof(ms_tformat), "%s", CLIB_LOG_TFORMAT_0 );
    } // if

    if ( ai_maxfile >= 0 ) mi_maxfile = ai_maxfile;
    else mi_maxfile = 5;

    if ( ai_maxsize >= 0 ) mi_maxsize = ai_maxsize; // Byte
    else mi_maxsize = CLIB_LOG_SIZE_NORM;

    if ( ai_level >= CLIB_LOG_LEV_ERROR ) mi_level = ai_level;
    else mi_level = CLIB_LOG_LEV_ERROR;

    if ( NULL != as_logtype ) {
        snprintf( ms_logtype, sizeof(ms_logtype), "%s", as_logtype );
    } else {
        snprintf( ms_logtype, sizeof(ms_logtype), "%s", CLIB_LOG_TYPE_NOR );
    } // if

    if ( NULL != as_eol ) {
        snprintf( ms_eol, sizeof(ms_eol), "%s", as_eol );
    } else {
        snprintf( ms_eol, sizeof(ms_eol), "%s", CLIB_LOG_EOL_LF );
    } // if

    //set_coloring_file
    set_coloring_file(as_coloring);
}
/* }}} */

 
/* {{{ clib_clog::~clib_log() */
/**
 * Destory clib_log class.
 */
clib_log::~clib_log( void )
{
}
/* }}} */


/* {{{ int clib_clog::set_coloring_file() */
/**
 * Set the coloring configure filename.
 *
 * @param   as_file     config filename.
 * @return              0: succ, !=0: fail.
 */

int clib_log::set_coloring_file(const char *as_coloring)
{
    if (NULL == as_coloring) return -1;

    setUins.clear();

    char achLine[20];
    FILE *fp_coloring = fopen(as_coloring, "r");
    if (fp_coloring == NULL)
    {
        mb_coloring = false;
        return -1;
    }

    else
    {
        uint32_t uin;
        while (fgets(achLine, sizeof(achLine) - 1, fp_coloring))
        {
            uin = atouin(achLine);
            if (clib_uin_is_valid(uin))
            {
                setUins.insert(uin);
            }
        }

        if (setUins.empty())
        {
            mb_coloring = false;
        }

        else
        {
            mb_coloring = true;
        }
    }

    fclose(fp_coloring);
    return 0;
}
/* }}} */


/* {{{ int clib_clog::set_file() */
/**
 * Set the log filename.
 *
 * @param   as_file     Log filename.
 * @param   ai_pathtype Log filename path type (define CLIB_LOG_PATH_xxx).
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_file( const char *as_file,
                        int         ai_pathtype )
{
    if ( NULL == as_file ) return(-1);

    //memset( ms_file,   0, sizeof(ms_file) );

    if ( ai_pathtype == CLIB_LOG_PATH_RELATIVE ) {
        snprintf( ms_file, sizeof(ms_file), "%s/%s",
                    CLIB_LOG_PATH, as_file );
    } else {
        snprintf( ms_file, sizeof(ms_file), "%s", as_file );
    } // if

    //init semopher
    int fd = open( ms_file, O_WRONLY|O_APPEND|O_CREAT, 0666 ); 
    if (fd > 0)
    {
        close(fd);
    }
    key_t ipc_key = ftok( ms_file, 0 );
    if (mi_semopen && ipc_key > 0)
    {
        createSem(ipc_key, &mi_semid, NULL);
    }

    return(0);
}
/* }}} */


/* {{{ int clib_clog::set_timeformat() */
/**
 * Set time format.
 *
 * @param   as_tformat  Time format, also see strftime() function use.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_timeformat( const char *as_tformat )
{
    if ( NULL == as_tformat ) return(-1);
    //memset( ms_tformat, 0, sizeof(ms_tformat) );
    snprintf( ms_tformat, sizeof(ms_tformat), "%s", as_tformat );
    return(0);
}
/* }}} */


/* {{{ int clib_clog::set_level() */
/**
 * Set log level.
 *
 * @param   ai_level    Log level.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_level( int ai_level )
{
    if ( ai_level < CLIB_LOG_LEV_ERROR ) return(-1);
    mi_level = ai_level;
    return(0);
}
/* }}} */


/* {{{ int clib_clog::set_maxfile() */
/**
 * Set max file.
 *
 * @param   ai_maxfile  Max log files.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_maxfile( int ai_maxfile )
{
    if ( ai_maxfile < 0 ) return(-1);
    mi_maxfile = ai_maxfile;
    return(0);
}
/* }}} */


/* {{{ int clib_clog::set_maxsize() */
/**
 * Set max size.
 *
 * @param   ai_maxsize  Max log size.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_maxsize( int ai_maxsize )
{
    if ( ai_maxsize < 0 ) return(-1);
    mi_maxsize = ai_maxsize;
    return(0);
}
/* }}} */


/* {{{ int clib_clog::set_logtype() */
/**
 * Set Log suffix type.
 *
 * @param   as_logtype  Log filename type, define in CLIB_LOG_TYPE_x.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_logtype( const char *as_logtype )
{
    if ( NULL != as_logtype ) {
        snprintf( ms_logtype, sizeof(ms_logtype), "%s", as_logtype );
    } else {
        snprintf( ms_logtype, sizeof(ms_logtype), "%s", CLIB_LOG_TYPE_NOR );
    } // if
    return( CLIB_LOG_OK );
}
/* }}} */


/* {{{ int clib_clog::set_eol() */
/**
 * Set the end of line.
 *
 * @param   as_eol      End of line.
 * @return              0: succ, !=0: fail.
 */
int clib_log::set_eol( const char *as_eol )
{
    if ( NULL == as_eol ) return(-1);
    //memset( ms_eol, 0, sizeof(ms_eol) );
    snprintf( ms_eol, sizeof(ms_eol), "%s", as_eol );
    return(0);
}   
/* }}} */


/* {{{ int clib_clog::write() */
/**
 * Write log.
 *
 * @param   as_msg      Message info.
 * @param   ai_level    Level num.
 * @return              0: succ, !=0: fail.
 */
int clib_log::write( const char *as_msg,
                     int        ai_level,
                     const UIN_TYPE ai_uin)
{
    time_t t;
    struct tm tm_re;
    struct tm *ptm = NULL;
    char   s_buf[CLIB_LOG_SLEN_NORM]  = { 0 };
    char   s_msg[CLIB_LOG_SLEN_LONG]  = { 0 };
    char   s_file[CLIB_LOG_SLEN_LONG] = { 0 };
    int    i_ret = 0;

    time( &t );
    ptm = localtime_r( &t, &tm_re );

    if ( ai_level > mi_level && (ai_uin == 0 || mb_coloring == false) ) {
        i_ret = 0; //coloring is open or not; if it's open, no level is care.
    } else if ( strlen(ms_file) <= 0
                    || NULL == as_msg
                    || strlen(as_msg) <= 0 ) {
        i_ret = -1;
    } else {
    
        if (ai_uin == 0 || mb_coloring == false){ //do not coloring
            if ( strlen(ms_tformat) > 0 ) {
                strftime( s_buf, sizeof(s_buf), ms_tformat, ptm );
                snprintf( s_msg, sizeof(s_msg), "%s%s%s", s_buf, as_msg, ms_eol );
            } else {
                snprintf( s_msg, sizeof(s_msg), "%s%s", as_msg, ms_eol );
            } // if
        }

        else{

            if (setUins.find(ai_uin) == setUins.end())
            {
                return 0;
            }
        
            if ( strlen(ms_tformat) > 0 ) {
                strftime( s_buf, sizeof(s_buf), ms_tformat, ptm );
                snprintf( s_msg, sizeof(s_msg), "%s[%u]%s%s", s_buf, ai_uin, as_msg, ms_eol );
            } else {
                snprintf( s_msg, sizeof(s_msg), "[%u]%s%s", ai_uin, as_msg, ms_eol );
            } // if
        }

        strftime( s_buf,  sizeof(s_buf),  ms_logtype, ptm );
        snprintf( s_file, sizeof(s_file), "%s%s", ms_file, s_buf );
    
        //lock
        if (mi_semopen) {
            SemLock(mi_semid, NULL);
        }
        
        this->check_file( s_file );
        i_ret = clib_log_write( s_file, s_msg, strlen(s_msg) );

        //unlock
        if (mi_semopen) {
            SemUnlock(mi_semid, NULL);
        }

    } // if ( ai_level > mi_level )
    return( i_ret );
}
/* }}} */


/* {{{ int clib_clog::apwrite() */
/**
 * Write log using va_list.
 *
 * @param   ai_level        Level num.
 * @param   as_msgformat    Message format.
 * @param   ast_ap          Message va_list.
 * @return                  >= 0: succ, < 0: fail.
 */
int clib_log::apwrite( const int     ai_level,
                       const char    *as_msgformat, 
                       va_list ast_ap,
                       const UIN_TYPE ai_uin)
{    
    char    s_msg[CLIB_LOG_SLEN_LONG];    
    int     i_ret = 0;

    if ( NULL == as_msgformat || strlen(as_msgformat) <= 0 ) {
        return(-1);
    } // if

    //memset( s_msg, 0, sizeof(s_msg) );
    i_ret = vsnprintf( s_msg, sizeof(s_msg), as_msgformat, ast_ap );
    if ( i_ret < 0 ) {
        return( i_ret );
    } // if   

    i_ret = this->write( s_msg, ai_level, ai_uin );
    return( i_ret );
}
/* }}} */


/* {{{ int clib_clog::vwrite() */
/**
 * Write log with variable param.
 *
 * @param   ai_level        Level num.
 * @param   as_msgformat    Message format.
 * @return                  >= 0: succ, < 0: fail.
 */
int clib_log::vwrite( const int  ai_level,
                      const char *as_msgformat, 
                      ... )
{    
    int     i_ret = 0;
    va_list ap;

    if ( NULL == as_msgformat || strlen(as_msgformat) <= 0 ) {
        return(-1);
    } // if

    va_start( ap, as_msgformat );  
    i_ret = this->apwrite( ai_level, as_msgformat, ap );
    va_end( ap );
    return( i_ret );
}
/* }}} */

int clib_log::logMsg( const char *as_msgformat , ... )
{    
    int     ai_level = CLIB_LOG_LEV_INFO;
    int     i_ret = 0;
    va_list ap;

    if ( NULL == as_msgformat || strlen(as_msgformat) <= 0 ) {
        return(-1);
    } // if

    va_start( ap, as_msgformat );  
    i_ret = this->apwrite( ai_level, as_msgformat, ap );
    va_end( ap );
    return( i_ret );
}


/* {{{ int clib_clog::vwrite() */
/**
 * Write log with variable param for colored uin. 
 * If @ai_uin is not contained in the conf file, this uin will not be logged.
 *
 * @param   ai_level        Level num.
 * @param   as_msgformat    Message format.
 * @return                  >= 0: succ, < 0: fail.
 */
int clib_log::vwrite( const int  ai_level,
                      const UIN_TYPE ai_uin,
                      const char *as_msgformat, 
                      ... )
{    
    int     i_ret = 0;
    va_list ap;

    if ( NULL == as_msgformat || strlen(as_msgformat) <= 0 ) {
        return(-1);
    } // if

    va_start( ap, as_msgformat );  
    i_ret = this->apwrite( ai_level, as_msgformat, ap, ai_uin );
    va_end( ap );
    return( i_ret );
}
/* }}} */

#endif

