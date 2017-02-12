//---------------------------------------------------------------------------

#ifndef IFStageH
#define IFStageH
#define TREE_SIZE 64

///////////////////////////////////////////////////
//      Inversion Frequency Stage Functions      //
///////////////////////////////////////////////////
int      * IFEncode  (short *bwt_seq, int *F, int *T, short alphaLen);
short    * IFDecode_A(int  *I, int *F, int *T, short alphaLen);
short    * IFDecode_D(int  *I, int *F, int *T, short alphaLen);

/////////////////////////////////////
//   For the Forward IF Algorithm  //
/////////////////////////////////////
typedef struct t_node_f {
	short     i;
	int      ai,
			pai;
	int *  posi;
	struct t_node_f *l_branch,
					*r_branch;

} tree_node;
tree_node * initialize_tree(int low, int high, int **group_ptrs);
void traverse_nodes(tree_node *node, int &sum, short symbol);
void free_tree(tree_node *node);

///////////////////////////////////////////////////////////////
//   For the Inverse IF Algorithm: list of symbols availabe  //
///////////////////////////////////////////////////////////////
typedef struct list_node {
	short symbol;
	struct list_node *next,
					 *prev;
} symbol_list;
symbol_list * create_alphabet(short alphaLen);
symbol_list * delete_symbol(symbol_list *node);

////////////////////////////////////////////////////////////////
//     For the Inverse IF Algorithm:    AVL trees             //
////////////////////////////////////////////////////////////////
typedef struct avl_node_struct {
   short val;
   int cnt;
   int bal;
   avl_node_struct *left, *right, *par;
} avl_node;
void inorder_traverse_avl_tree (avl_node *&, short *&p);
void insert_node_in_avl_tree( avl_node * &, short , int);
void rotate_ll(avl_node * & rt, avl_node *, avl_node * );
void rotate_rr(avl_node * & rt, avl_node *, avl_node * );
void rotate_lr(avl_node * & rt, avl_node *, avl_node *, avl_node * );
void rotate_rl(avl_node * & rt, avl_node *, avl_node *, avl_node * );
#endif



