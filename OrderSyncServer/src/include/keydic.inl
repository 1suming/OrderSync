
template<class T>
CKeydic<T>::CKeydic(void)
{
    m_pRoot = NULL;
    m_pRootCase = NULL;
    m_pCurItem = NULL;
    m_pCurItemCase = NULL;
}

template <class T>
CKeydic<T>::~CKeydic(void)
{
    release_items();
}

template <class T>
void CKeydic<T>::loop_release(struct itemdata<T>* pItem)
{
	if(pItem == NULL)
		return;
	else
	{
		for(int i=0; i<ITEM_NUM; i++)
		{
			loop_release(pItem->_arChild[i]);
		}

        if(NULL != pItem->_element)
        {
            pItem->_element = NULL;
        }
        
		DELETE (pItem);
	}
}

template <class T>
int CKeydic<T>::strtopos(char ch)
{
	for(int i=0; i<ITEM_NUM; i++)
	{
		if(ch == keys[i])
			return i;
	}

	return -1;
}

/*
input:nflag
    0:create normal tree
    1: create the tree with no case
    2: create both trees
*/
template <class T>
int CKeydic<T>::init_root(int nflag)
{
    if(nflag == 0 || nflag == 2)
    {	
        itemdata<T>* pItem = NULL;
        NEW (itemdata<T>, pItem);

        if (NULL == pItem)
        {
            return -1;
        }

        pItem->_pParent = NULL;
        pItem->_element = NULL;
        for(int i=0; i<ITEM_NUM; i++)
        {
            pItem->_arChild[i] = NULL;
        }
        m_pRoot = pItem;
    }

    if(nflag == 1 || nflag == 2)
    {
        itemdata<T>* pItem2 = NULL;
        NEW (itemdata<T>, pItem2);

        if (NULL == pItem2)
        {
            return -1;
        }

        pItem2->_pParent = NULL;
        pItem2->_element = NULL;
        for(int i=0; i<ITEM_NUM; i++)
        {
            pItem2->_arChild[i] = NULL;
        }

        m_pRootCase = pItem2;
    }

    return 0;
}

template <class T>
void CKeydic<T>::release_items(void)
{
	if(m_pRoot != NULL)
	{
		for(int i=0; i<ITEM_NUM; i++)
		{
			loop_release(m_pRoot->_arChild[i]);
		}

        if(NULL != m_pRoot->_element)
        {
            m_pRoot->_element = NULL;
        }
        
		DELETE (m_pRoot);
	}

	if(m_pRootCase != NULL)
	{
		for(int i=0; i<ITEM_NUM; i++)
		{
			loop_release(m_pRootCase->_arChild[i]);
		}
		
        if(NULL != m_pRootCase->_element)
        {
            m_pRootCase->_element = NULL;
        }
        
        DELETE (m_pRootCase);
	}

}

template <class T>
int CKeydic<T>::add_key(const char* pBuf, int nLen, T* pElement)
{
    struct itemdata<T> *pLastItem = NULL;
    struct itemdata<T> *pLastCaseItem = NULL;
    
	if(m_pRoot != NULL)
	{
		m_pCurItem = m_pRoot;

		for(int i=0; i<nLen; i++)
		{
			int nPos = strtopos(pBuf[i]);
            //if not found finished
            if(nPos == -1)
                goto exit;
            
			if(m_pCurItem != NULL)
			{
				if(m_pCurItem->_arChild[nPos] == NULL)
				{
					itemdata<T> *pItem = NULL;
                    NEW (itemdata<T>, pItem);

                    if (NULL == pItem)
                    {
                        return RETCODE_FAIL;
                    }

					pItem->_ch = pBuf[i];
                    pItem->_element = NULL;
                    
					m_pCurItem->_arChild[nPos] = pItem;
					pItem->_pParent = m_pCurItem;
					m_pCurItem = pItem;
				}
				else
				{
					m_pCurItem = m_pCurItem->_arChild[nPos];
                    m_pCurItem->_ch = pBuf[i];
					//m_pCurItem->_element = pElement;
				}

                if(m_pCurItem != NULL)
                    pLastItem = m_pCurItem;
			}
		}
	}

	if(m_pRootCase != NULL)
	{
		m_pCurItemCase = m_pRootCase;

		for(int i=0; i<nLen; i++)
		{
			int nPos = strtopos(tolower(pBuf[i]));
			if(nPos == -1)
                goto exit;
            
            if(m_pCurItemCase != NULL)
			{
				if(m_pCurItemCase->_arChild[nPos] == NULL)
				{
					itemdata<T>* pItem = NULL;
                    NEW (itemdata<T>, pItem);

                    if (NULL == pItem)
                    {
                        return RETCODE_FAIL;
                    }

					pItem->_ch = tolower(pBuf[i]);
					pItem->_element = NULL;
                    
                    m_pCurItemCase->_arChild[nPos] = pItem;
					pItem->_pParent = m_pCurItemCase;
					m_pCurItemCase = pItem;
				}
				else
				{
					m_pCurItemCase = m_pCurItemCase->_arChild[nPos];
                    m_pCurItemCase->_ch = tolower(pBuf[i]);
					//m_pCurItemCase->_element = pElement;
				}

                if(m_pCurItemCase != NULL)
                    pLastCaseItem = m_pCurItemCase;
			}
		}	
	}

exit:
    if(pLastItem != NULL)
    {
        if(pLastItem->_element == NULL)
            pLastItem->_element = pElement;
        else
            return RETCODE_NORMALTREE_DUP;
    }

    if(pLastCaseItem != NULL)
    {
        if(pLastCaseItem->_element == NULL)
            pLastCaseItem->_element = pElement;
        else
            return RETCODE_CASETREE_DUP;
    }

    return RETCODE_SUCC;
}

template <class T>
int CKeydic<T>::search_key(const char* pBuf, int nLen, T **pElement, char* pOutBuf)
{
	int     nIndex  = 0;
    bool    bMatch   = false;

	if(m_pRoot == NULL)
		return -1;
	else
		m_pCurItem = m_pRoot;

	for(int i=0; i<nLen; i++)
	{
		int nPos = strtopos(pBuf[i]);
		if(nPos == -1)
            break;
        
        if(m_pCurItem->_arChild[nPos] != NULL)
		{
			if(m_pCurItem->_arChild[nPos]->_ch == pBuf[i])
			{
				//collect the character
				bMatch = true;
				
                if(pOutBuf != NULL)
					pOutBuf[nIndex++] = m_pCurItem->_arChild[nPos]->_ch;
				
                *pElement = m_pCurItem->_arChild[nPos]->_element;
                
                m_pCurItem = m_pCurItem->_arChild[nPos];
			}		
			else
			{
				break;
			}
		}
		else
		{
			//end tree, stop here
			break;
		}
	}

	if(bMatch == true)
	{
	    if(pOutBuf != NULL)	
            pOutBuf[nIndex] = '\0';
		
        return 0;
	}	

	return -1;
}

template <class T>
int CKeydic<T>::search_ncase_key(const char* pBuf, int nLen, T** pElement, char* pOutBuf)
{                                                                                                 
    int       nIndex = 0;
    bool      bMatch = false;
    
    if(m_pRootCase == NULL)              
        return -1;
    else
        m_pCurItemCase = m_pRootCase;

    for(int i=0; i<nLen; i++)
    {
        int nPos = strtopos(tolower(pBuf[i]));
        if(nPos == -1)
            break;
        
        if(m_pCurItemCase->_arChild[nPos] != NULL)
        {
            if(m_pCurItemCase->_arChild[nPos]->_ch == tolower(pBuf[i]))
            {
                bMatch = true;
                
                if(pOutBuf != NULL)
                    pOutBuf[nIndex++] = m_pCurItemCase->_arChild[nPos]->_ch;
                *pElement = m_pCurItemCase->_arChild[nPos]->_element;
                
                m_pCurItemCase = m_pCurItemCase->_arChild[nPos];
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if(bMatch == true)
    {
        if(pOutBuf != NULL)
            pOutBuf[nIndex] = '\0';
        return 0;
    }

    return -1;
}

