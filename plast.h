struct PlastView 
{
};

struct PlastTree
{
};

struct PlastView *past_open      (struct PlastTree *tree);
struct PlastView *past_open_here (struct PlastTree *tree);
void              past_inspect   (struct PlastTree *tree);

