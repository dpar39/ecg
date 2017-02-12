#include "IFStage.h"
#include "CommonDef.h"

///////////////////////////////////////////////////////////
//        IF Stage Encoding and Decoding Functions       //
///////////////////////////////////////////////////////////
int* Transform;

int* IFEncode(short* bwt_seq, int* F, int* T, short alphaLen)
{
    unsigned IFLen = sum(F, alphaLen);
    int* IF = new int[IFLen];
    int** IFptrs = new int*[alphaLen]; //free it
    unsigned k;
    int sum;
    k = 1;
    IFptrs[0] = IF;
    while (k < alphaLen)
    {
        IFptrs[k++] = IFptrs[k - 1] + F[k - 1];
    }

    tree_node* root = initialize_tree(0, alphaLen - 1, IFptrs);
    for (k = 0; k < IFLen; k++)
    {
        sum = 0;
        traverse_nodes(root, sum, T[bwt_seq[k]]);
    }
    delete []IFptrs;
    free_tree(root); //free tree
    return IF;
}

short* IFDecode_A(int* I, int* F, int* T, short alphaLen)
{
    avl_node* root = nullptr;
    unsigned Len = sum(F, alphaLen);
    short* M = new short[Len];
    short i = 0;
    unsigned k;
    short* M_ptr = M; //pointer to M;
    short symbol;
    Transform = T;

    //Make array of pointers to I
    int** Iptrs = new int*[alphaLen];
    int* Ivals = new int[alphaLen];
    int* LastSymbolRanks = new int[F[alphaLen - 1]];
    reset_vector(LastSymbolRanks, F[alphaLen - 1]);

    Iptrs[0] = I;
    for (i = 0; i < alphaLen - 1; i++)
    {
        Iptrs[i + 1] = Iptrs[i] + F[i];
        Ivals[i] = *(Iptrs[i]);
    }
    Iptrs[alphaLen - 1] = LastSymbolRanks; //zero padd for last symbol
    Ivals[alphaLen - 1] = 0;
    symbol_list *Sym = create_alphabet(alphaLen), *Begin = Sym;

    k = TREE_SIZE;
    unsigned to_process = Len;
    while (to_process)
    {
        if (Ivals[symbol = Sym->symbol] < k)
        {
            insert_node_in_avl_tree(root, symbol, Ivals[symbol]);
            --to_process;
            --k;
            F[symbol]--;
            ++Iptrs[symbol];
            Ivals[symbol] += *Iptrs[symbol];
            if (!F[symbol])
            {
                if (Sym == Begin)
                    Begin = Sym = delete_symbol(Sym);
                else
                    Sym = delete_symbol(Sym);
            }
        }
        else
        {
            Ivals[symbol] -= k;
            Sym = Sym->next;
        }
        if (!k || !to_process)
        {
            k = TREE_SIZE;
            inorder_traverse_avl_tree(root, M_ptr);
            if (!to_process)
                break;
            Sym = Begin;
        }
    }
    delete [] Iptrs;
    delete []Ivals;
    delete []LastSymbolRanks;
    return M;
}

short* IFDecode_D(int* I, int* F, int* T, short alphaLen)
{
    unsigned Len = sum(F, alphaLen);
    short* M = new short[Len];
    short sym = 0;
    short* M_ptr = M;

    //Make array of pointers to I
    int** Iptrs = new int*[alphaLen];
    int* Ivals = new int[alphaLen];
    int* LastSymbolRanks = new int[F[alphaLen - 1]];
    reset_vector(LastSymbolRanks, F[alphaLen - 1]);

    Iptrs[0] = I;
    for (sym = 0; sym < alphaLen - 1; ++sym)
    {
        Iptrs[sym + 1] = Iptrs[sym] + F[sym];
        Ivals[sym] = *(Iptrs[sym]);
    }
    Iptrs[alphaLen - 1] = LastSymbolRanks; //zero padd for last symbol
    Ivals[alphaLen - 1] = 0;
    symbol_list *Sym = create_alphabet(alphaLen), *Begin = Sym;

    unsigned to_process = Len;
    while (to_process)
    {
        if (!Ivals[sym = Sym->symbol])
        {
            --to_process;
            --F[sym];
            ++Iptrs[sym];
            Ivals[sym] += *Iptrs[sym];
            if (!F[sym])
                if (Sym == Begin)
                    Begin = Sym = delete_symbol(Sym);
                else
                    Sym = delete_symbol(Sym);
            *M_ptr++ = T[sym];
            Sym = Begin;
        }
        else
        {
            --Ivals[sym];
            Sym = Sym->next;
        }
    }
    delete [] Iptrs;
    delete []Ivals;
    delete []LastSymbolRanks;
    return M;
}

//////////////////////////////////////////////////////////////
//   Forward IF Algorithm functions of structured data      //
//////////////////////////////////////////////////////////////
tree_node* initialize_tree(int low, int high, int** group_ptrs)
{
    tree_node* the_node = new tree_node;
    the_node->i = (low + high) / 2;
    the_node->ai = 0;
    the_node->pai = 0;
    the_node->posi = group_ptrs[the_node->i];
    the_node->l_branch = the_node->r_branch = nullptr;

    if (low < high)
    {
        the_node->l_branch = initialize_tree(low, the_node->i - 1, group_ptrs);
        the_node->r_branch = initialize_tree(the_node->i + 1, high, group_ptrs);
    }
    return the_node;
}

void traverse_nodes(tree_node* node, int& sum, short symbol)
{
    if (node->i > symbol)
    {
        sum += node->ai;
        traverse_nodes(node->l_branch, sum, symbol);
    }
    else if (node->i < symbol)
    {
        ++(node->ai);
        traverse_nodes(node->r_branch, sum, symbol);
    }
    else
    {
        sum += node->ai;
        *(node->posi)++ = sum - node->pai;
        ++(node->ai);
        node->pai = sum + 1;
    }
}

void free_tree(tree_node* node)
{
    if (node->l_branch)
    {
        free_tree(node->l_branch);
    }
    if (node->r_branch)
    {
        free_tree(node->r_branch);
    }
    delete node;
}

//////////////////////////////////////////////////////////////
//   Inverse IF Algorithm functions of structured data      //
//////////////////////////////////////////////////////////////
/*  Function to create a linked list to rotate over the symbol alphabet */
symbol_list* create_alphabet(short alphaLen)
{
    symbol_list *begin = new symbol_list, *tmp = begin;
    begin->symbol = 0;
    for (short i = 1; i < alphaLen; i++)
    {
        tmp->next = new symbol_list;
        tmp->next->prev = tmp;
        tmp = tmp->next;
        tmp->symbol = i;
    }
    tmp->next = begin;
    begin->prev = tmp;
    return begin;
}

/*  Function to delete a node (symbol) from the symbol linked list (alphabet) */
symbol_list* delete_symbol(symbol_list* node)
{
    symbol_list* tmp = node->next;
    tmp->prev = node->prev;
    node->prev->next = tmp;
    delete node;
    return tmp;
}

////////////////////////////////
//  AVL Trees implementations //
////////////////////////////////
void insert_node_in_avl_tree(avl_node* & rt, short val, int cnt)
{
    avl_node *ptrB, *ptrA, *ptrC, *ptrD;
    int dir1 = 0, dir2 = 0;

    ptrB = new avl_node;
    ptrB->left = nullptr;
    ptrB->right = nullptr;
    ptrB->par = nullptr;
    ptrB->val = val;
    ptrB->cnt = cnt;
    ptrB->bal = 0;

    if (rt == nullptr)
    { // first node created
        rt = ptrB;
        ptrB->par = rt;
    }
    else
    { //1
        ptrA = ptrC = rt; // now find insertion location

        while (ptrA != nullptr)
        {
            if (ptrB->cnt < ptrA->cnt)
            {
                dir2 = 1;
                ptrC = ptrA; // C is used as temp backup
                ptrA->cnt -= 1; //reduce count by 1
                ptrA = ptrA->left; // go left
                if (ptrA != nullptr)
                    ptrA->par = ptrC;
            }
            else
            {
                dir2 = 0;
                ptrB->cnt -= ptrA->cnt;
                ptrC = ptrA; // C is used as temp backup
                ptrA = ptrA->right; // go right
                if (ptrA != nullptr)
                    ptrA->par = ptrC;
            }
        } // while ptrA != nullptr

        // adjust ptrC, insert ptrB new node to tree
        if (dir2)
        {
            ptrC->left = ptrB;
            ptrB->par = ptrC;
        }
        else
        {
            ptrC->right = ptrB;
            ptrB->par = ptrC;
        }
        ///// Adjust Balance factors
        ptrA = ptrB->par;
        ptrD = ptrA->par;
        if (ptrA->left == ptrB)
        { // inserted to left
            ++ptrA->bal;
            dir1 = 0;
        } // end if
        else
        { // inserted right
            --ptrA->bal;
            dir1 = 1;
        }

        while (ptrA != rt && (ptrA->bal == 1 || ptrA->bal == -1))
        {
            dir2 = dir1; // hold previous direction info.

            ptrB = ptrA;
            ptrA = ptrA->par;
            if (ptrA == nullptr) break;

            if (ptrA->left == ptrB)
            {
                ptrA->bal += 1;
                dir1 = 0;
            } // end if
            else
            {
                ptrA->bal -= 1;
                dir1 = 1;
            }
        } //end while ptrA != rt

        // if balance is 2/-2 Balancing is needed
        // determine directions of insertion and call approp. rotation
        if (ptrA != nullptr)
        {
            if (ptrA->bal == 2 || ptrA->bal == -2)
            {
                if (dir1 == 0)
                {
                    if (dir2 == 0)
                        rotate_ll(rt, ptrA, ptrB);
                    else
                        rotate_lr(rt, ptrA, ptrB, ptrB->right);
                }
                else
                {
                    if (dir2 == 0)
                        rotate_rl(rt, ptrA, ptrB, ptrB->left);
                    else
                        rotate_rr(rt, ptrA, ptrB);
                }
            } // end if ptrA->bal 2 or -2
        } // ptrA !=nullptr
    } // else 1
} //end insert
////////////////////////////////
// Traverse AVL Tree INORDER  //
////////////////////////////////
void inorder_traverse_avl_tree(avl_node* & root, short* & p)
{
    if (!root) return;
    inorder_traverse_avl_tree(root->left, p);
    *p++ = Transform[root->val];
    inorder_traverse_avl_tree(root->right, p);
    delete root;
    root = nullptr;
}

////////////////////////////////
// AVL Tree Rotations 	      //
////////////////////////////////
void rotate_rr(avl_node* & rt, avl_node* ptrA, avl_node* ptrB)
{
    avl_node* ptrT;

    ptrA->right = ptrB->left;
    ptrB->left = ptrA;

    if (ptrA == rt)
    {
        rt = ptrB;
    }
    else
    {
        ptrT = ptrA->par;
        if (ptrT->left == ptrA)
            ptrT->left = ptrB;
        else
            ptrT->right = ptrB;
    }
    ptrA->bal = ptrB->bal = 0;
    ptrB->cnt += ptrA->cnt;
} // end rotate_rr
void rotate_ll(avl_node* & rt, avl_node* ptrA, avl_node* ptrB)
{
    avl_node* ptrT;

    ptrA->left = ptrB->right;
    ptrB->right = ptrA;

    if (ptrA == rt)
    {
        rt = ptrB;
    }
    else
    {
        ptrT = ptrA->par;
        if (ptrT->left == ptrA)
            ptrT->left = ptrB;
        else
            ptrT->right = ptrB;
    }
    ptrA->bal = ptrB->bal = 0;
    ptrA->cnt -= ptrB->cnt;
} // end rotate_ll
void rotate_rl(avl_node* & rt, avl_node* ptrA, avl_node* ptrB, avl_node* ptrC)
{
    avl_node* ptrT;

    ptrB->left = ptrC->right;
    ptrC->right = ptrB;
    ptrA->right = ptrC->left;
    ptrC->left = ptrA;

    if (ptrA == rt)
        rt = ptrC;
    else
    {
        ptrT = ptrA->par;
        if (ptrT->left == ptrA)
            ptrT->left = ptrC;
        else
            ptrT->right = ptrC;
    }
    ptrB->cnt -= ptrC->cnt;
    ptrC->cnt += ptrA->cnt;
    if (ptrC->bal == 0)
        ptrA->bal = ptrB->bal = 0;
    else
    {
        if (ptrC->bal == 1)
        {
            ptrA->bal = 0;
            ptrB->bal = -1;
            ptrC->bal = 0;
        }
        else
        {
            ptrA->bal = 1;
            ptrB->bal = 0;
            ptrC->bal = 0;
        }
    } //end else
} // end rotate_rl
void rotate_lr(avl_node* & rt, avl_node* ptrA, avl_node* ptrB, avl_node* ptrC)
{
    avl_node* ptrT;

    ptrB->right = ptrC->left;
    ptrC->left = ptrB;
    ptrA->left = ptrC->right;
    ptrC->right = ptrA;

    if (ptrA == rt)
        rt = ptrC;
    else
    {
        ptrT = ptrA->par;
        if (ptrT->left == ptrA)
            ptrT->left = ptrC;
        else
            ptrT->right = ptrC;
    }
    ptrC->cnt += ptrB->cnt;
    ptrA->cnt -= ptrC->cnt;
    if (ptrC->bal == 0)
        ptrA->bal = ptrB->bal = 0;
    else
    {
        if (ptrC->bal == 1)
        {
            ptrA->bal = -1;
            ptrB->bal = 0;
            ptrC->bal = 0;
        }
        else
        {
            ptrA->bal = 0;
            ptrB->bal = 1;
            ptrC->bal = 0;
        }
    }
} // end rotate_lr
