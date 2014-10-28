#ifndef _KEYDIC_H_
#define _KEYDIC_H_

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memcheck.h>

#define ITEM_NUM	            100

#define RETCODE_SUCC            0
#define RETCODE_FAIL           -1
#define RETCODE_NORMALTREE_DUP -2
#define RETCODE_CASETREE_DUP   -3

static const char keys[ITEM_NUM]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','`',' ','~','!','@','#','$','%','^','&','*','(',')','_','-','+','=','[',']','{','}','/','|','\'','"',';',':',',','<','>','.','?','\\','\r','\n','\0'};

template <class T> struct itemdata{
    itemdata(){
        _ch = 0x00;
        _element = NULL;
        _pParent = NULL;
        for(int i=0; i<ITEM_NUM; i++)
        {
            _arChild[i] = NULL;
        }
    }

    ~itemdata(){

    }

    char _ch;
	struct itemdata *_pParent;
	struct itemdata *_arChild[ITEM_NUM];
    T* _element;
};

template <class T>
class CKeydic
{

public:
    CKeydic();
    
    virtual ~CKeydic();
    
public:
    /*
     * intput: nflag 
     * 0: create tree with case 
     * 1: create tree with no case 
     * 2: create the two trees
     */
    int init_root(int nflag);

    /*
     *input: 
     *pBuf: string for building the dictionary; 
     *nLen: length of the string
     *pElement: object element
     */
    int add_key(const char* pBuf, int nLen, T* pElement);

    /*
     *input
     *pBuf: string for searching
     *nLen: the length of the string
     *output:
     *pElement: object element
     *pOutBuf: the maximum mathed string
     */
    int search_key(const char* pBuf, int nLen, T** pElement, char* pOutBuf = NULL);

    /*
     *input:
     *pBuf: string for searching
     *nLen: the length of the string

     *output:
     *pElement: object element
     *pOutBuf: the maximum mathed string
     */
    int search_ncase_key(const char* pBuf, int nLen, T** pElement, char* pOutBuf = NULL);

    void release_items(void);

protected:

    void loop_release(struct itemdata<T>* pItem);

    int strtopos(char ch);
    
private:
    
    struct itemdata<T>* m_pRoot;
    struct itemdata<T>* m_pRootCase;
    struct itemdata<T>* m_pCurItem;
    struct itemdata<T>* m_pCurItemCase;
};

#include "keydic.inl"

#endif
