// vim: set expandtab tabstop=4 shiftwidth=4 fdm=marker:
// +----------------------------------------------------------------------+
// | Club Library.                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 2005-2010 Tencent Inc. All Rights Reserved.            |
// +----------------------------------------------------------------------+
// | Authors: The Club Dev Team, ISRD, Tencent Inc.                       |
// |          harrychen <harrychen@tencent.com>                             |
// +----------------------------------------------------------------------+
// $Id$

/**
 * @version $Revision$
 * @author  $Author$
 * @date    $Date$
 * @brief   Club Library.
 */
#ifndef WIN32

#ifndef _CLIB_UIN_TYPE_H_
#define _CLIB_UIN_TYPE_H_

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t UIN_TYPE;

#define atouin(s) ((UIN_TYPE) (strtoul(s, NULL, 10) ) )

#define MIN_UIN 10000
#define MAX_UIN 0xffffffff
inline bool clib_uin_is_valid(UIN_TYPE uin)
{
    if (uin < MIN_UIN || uin > MAX_UIN)
    {
        return false;
    }

    return true;
}

#endif

#endif
