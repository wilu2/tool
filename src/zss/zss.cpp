#include "zss.h"
#include "XChar.h"
#include "AnnotatedTree.h"
#include "../xie_debug.h"


namespace xwb
{

using std::vector;



int min_num3(int n1, int n2, int n3, int& max_val)
{
    int index = 0;
    max_val = n1;

    if(n2 < max_val)
    {
        max_val = n2;
        index = 1;
    }

    if(n3 < max_val)
    {
        max_val = n3;
        index = 2;
    }
    return index;
}


int zss_ted(Node<DocNode>* A,  Node<DocNode>* B, vector<TEDOp<DocNode>>& ops)
{
    TIME_COST_FUNCTION;
    DocNodeCostModel cost_model;
    AnnotatedTree<DocNode> atA(A);
    AnnotatedTree<DocNode> atB(B);
    
    int size_a = atA.nodes.size();
    int size_b = atB.nodes.size();

    std::vector<std::vector<int> > treedists(size_a, std::vector<int>(size_b, 0));
    vector<vector<vector<TEDOp<DocNode> > > > operations(size_a, vector<vector<TEDOp<DocNode> > >(size_b));

    auto& Al = atA.lmds;
    auto& Bl = atB.lmds;
    auto& An = atA.nodes;
    auto& Bn = atB.nodes;

    auto treedist = [&](int i, int j) {
        int ioff = Al[i] - 1;
        int joff = Bl[j] - 1;
        int m = i - ioff + 1; 
        int n = j - joff + 1;
            
        vector<vector<int>> fd(m, vector<int>(n, 0));
        vector<vector<vector<TEDOp<DocNode> > > > partial_ops(m, vector<vector<TEDOp<DocNode> > >(n));

        for(int x = 1;  x < m; x++)
        {
            auto node = An[x+ioff];
            fd[x][0] = fd[x-1][0] + cost_model.del(node);

            partial_ops[x][0] = partial_ops[x-1][0];
            partial_ops[x][0].emplace_back(DELETE, node, nullptr);
        }

        for(int y = 1; y < n; y++)
        {
            auto node = Bn[y+joff];
            fd[0][y] = fd[0][y-1] + cost_model.ins(node);

            partial_ops[0][y] = partial_ops[0][y-1];
            partial_ops[0][y].emplace_back(INSERT, nullptr, node);
        }

        for(int x = 1; x < m; x++)
        {
            for(int y = 1; y < n; y++)
            {
                auto node1 = An[x+ioff];
                auto node2 = Bn[y+joff];

                if(Al[i] == Al[x+ioff] && Bl[j] == Bl[y+joff])
                {
                    int min_index = min_num3(fd[x-1][y] + cost_model.del(node1), 
                                             fd[x][y-1] + cost_model.ins(node2),
                                             fd[x-1][y-1] + cost_model.ren(node1, node2),
                                             fd[x][y]);
                    if(min_index == 0)
                    {
                        partial_ops[x][y] = partial_ops[x-1][y];
                        partial_ops[x][y].emplace_back(DELETE, node1, nullptr);
                    }
                    else if(min_index == 1)
                    {
                        partial_ops[x][y] = partial_ops[x][y-1];
                        partial_ops[x][y].emplace_back(INSERT, nullptr, node2);
                    }
                    else
                    {
                        partial_ops[x][y] = partial_ops[x-1][y-1];
                        partial_ops[x][y].emplace_back((fd[x][y] == fd[x-1][y-1] ? NORMAL : CHANGE), node1, node2);
                    }

                    operations[x+ioff][y+joff] = partial_ops[x][y];
                    treedists[x+ioff][y+joff] = fd[x][y];
                }
                else
                {
                    int p = Al[x+ioff]-1-ioff;
                    int q = Bl[y+joff]-1-joff;

                    int min_index = min_num3(fd[x-1][y] + cost_model.del(node1), 
                                                fd[x][y-1] + cost_model.ins(node2),
                                                fd[p][q] + treedists[x+ioff][y+joff],
                                                fd[x][y]);
                    if(min_index == 0)
                    {
                        partial_ops[x][y] = partial_ops[x-1][y];
                        partial_ops[x][y].emplace_back(DELETE, node1, nullptr);
                    }
                    else if(min_index == 1)
                    {
                        partial_ops[x][y] = partial_ops[x][y-1];
                        partial_ops[x][y].emplace_back(INSERT, nullptr, node2);
                    }
                    else
                    {
                        partial_ops[x][y] = partial_ops[p][q];
                        partial_ops[x][y].insert(partial_ops[x][y].end(), operations[x+ioff][y+joff].begin(), operations[x+ioff][y+joff].end());
                    }
                }
            }
        }        
    };

    for(auto i : atA.keyroots)
    {
        for(auto j : atB.keyroots)
        {
            treedist(i, j);
        }
    }

    ops = operations[size_a-1][size_b-1];

    return treedists[size_a-1][size_b-1];
}

int zss_ted_backtrack(Node<DocNode>* A,  Node<DocNode>* B, vector<TEDOp<DocNode>>& ops)
{
    TIME_COST_FUNCTION;
    DocNodeCostModel cost_model;
    AnnotatedTree<DocNode> atA(A);
    AnnotatedTree<DocNode> atB(B);
    
    int size_a = atA.nodes.size();
    int size_b = atB.nodes.size();

    std::vector<std::vector<int> > treedists(size_a, std::vector<int>(size_b, 0));
    std::stack<std::vector<int>> stk;
    stk.push({size_a-1, size_b-1});


    auto& Al = atA.lmds;
    auto& Bl = atB.lmds;
    auto& An = atA.nodes;
    auto& Bn = atB.nodes;

    auto treedist = [&](int i, int j, bool backtrack) {
        
        int m = i - Al[i] + 2; // m = i - (AL[i]-1) + 1
        int n = j - Bl[j] + 2; // n = j - (BL[j]-1) + 1 
            
        vector<vector<int>> fd(m, vector<int>(n, 0));

        int ioff = Al[i] - 1;
        int joff = Bl[j] - 1;

        for(int x = 1;  x < m; x++)
        {
            auto node = An[x+ioff];
            fd[x][0] = fd[x-1][0] + cost_model.del(node);
        }

        for(int y = 1; y < n; y++)
        {
            auto node = Bn[y+joff];
            fd[0][y] = fd[0][y-1] + cost_model.ins(node);
        }

        for(int x = 1; x < m; x++)
        {
            for(int y = 1; y < n; y++)
            {
                auto node1 = An[x+ioff];
                auto node2 = Bn[y+joff];

                if(Al[i] == Al[x+ioff] && Bl[j] == Bl[y+joff])
                {
                    int min_index = min_num3(fd[x-1][y] + cost_model.del(node1), 
                                             fd[x][y-1] + cost_model.ins(node2),
                                             fd[x-1][y-1] + cost_model.ren(node1, node2),
                                             fd[x][y]);
                    treedists[x+ioff][y+joff] = fd[x][y];
                }
                else
                {
                    int p = Al[x+ioff]-1-ioff;
                    int q = Bl[y+joff]-1-joff;

                    int min_index = min_num3(fd[x-1][y] + cost_model.del(node1), 
                                                fd[x][y-1] + cost_model.ins(node2),
                                                fd[p][q] + treedists[x+ioff][y+joff],
                                                fd[x][y]);
                }
            }
        }  


        if(backtrack)
        {
            int x = m-1;
            int y = n-1;
            while(x > 0 && y > 0)
            {
                auto node1 = An[x+ioff];
                auto node2 = Bn[y+joff];

                if(Al[i] == Al[x+ioff] && Bl[j] == Bl[y+joff])
                {
                    int min_index = min_num3(fd[x - 1][y] + cost_model.del(node1),
                        fd[x][y - 1] + cost_model.ins(node2),
                        fd[x - 1][y - 1] + cost_model.ren(node1, node2),
                        fd[x][y]);

                    if (min_index == 0)
                    {
                        ops.emplace_back(DELETE, node1, nullptr);
                        x--;
                    }
                    else if (min_index == 1)
                    {
                        ops.emplace_back(INSERT, nullptr, node2);
                        y--;
                    }
                    else
                    {
                        ops.emplace_back((fd[x][y] == fd[x - 1][y - 1] ? NORMAL : CHANGE), node1, node2);
                        x--;
                        y--;
                    }
                }
                else
                {
                    int p = Al[x + ioff] - 1 - ioff;
                    int q = Bl[y + joff] - 1 - joff;

                    int min_index = min_num3(fd[x - 1][y] + cost_model.del(node1),
                        fd[x][y - 1] + cost_model.ins(node2),
                        fd[p][q] + treedists[x + ioff][y + joff],
                        fd[x][y]);


                    if (min_index == 0)
                    {
                        ops.emplace_back(DELETE, node1, nullptr);
                        x--;
                    }
                    else if (min_index == 1)
                    {
                        ops.emplace_back(INSERT, nullptr, node2);
                        y--;
                    }
                    else
                    {
                        stk.push({ x + ioff, y + joff });
                        x = p;
                        y = q;
                    }
                }
            }

            while(x>0)
            {
                auto node = An[x+ioff];
                ops.emplace_back(DELETE, node, nullptr);
                x--;
            }

            while(y>0)
            {
                auto node = Bn[y+joff];
                ops.emplace_back(INSERT, nullptr, node);
                y--;
            }
        }

    };

    for(auto i : atA.keyroots)
    {
        for(auto j : atB.keyroots)
        {
            treedist(i, j, false);
        }
    }

    while (stk.size())
    {
        auto idx = stk.top(); stk.pop();

        int i = idx[0];
        int j = idx[1];

        treedist(i, j, true);
    }

    return treedists[size_a-1][size_b-1];
}


} // namespace xwb
