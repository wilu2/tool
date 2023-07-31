#pragma once
#include "zss/Node.h"
#include <deque>
#include <map>
#include <stack>
#include "xie_debug.h"

namespace xwb
{


template<class D>
struct TmpNode
{
    Node<D>* node;
    int nid;
    std::deque<int> anc;

    TmpNode()
        :node(nullptr), nid(0){}

    TmpNode(Node<D>* d)
        :node(d), nid(0){}
};



template<class D>
class AnnotatedTree
{
public:
    AnnotatedTree(Node<D>* r)
        :root(r)
    {
        TIME_COST_FUNCTION;
        std::stack<TmpNode<D>> stk;
        std::stack<TmpNode<D>> pstk;

        TmpNode<D> tn(root);
        stk.push(tn);

        int j = 0;
        while(stk.size())
        {
            TmpNode<D> t = stk.top(); stk.pop();
            int nid = j;

            for(auto c : t.node->children)
            {
                TmpNode<D> sub_t;
                sub_t.node = c;
                sub_t.anc = t.anc;
                sub_t.anc.push_front(nid);
                stk.push(sub_t);
            }
            t.nid = nid;
            pstk.push(t);
            j++;
        }

        std::map<int, int> tmp_lmds;
        std::map<int, int> tmp_keyroots;

        int i = 0;
        int lmd = 0;
        while(pstk.size())
        {
            TmpNode<D> t_node = pstk.top(); pstk.pop();
            nodes.push_back(t_node.node);
            ids.push_back(t_node.nid);

            if(t_node.node->children.empty())
            {
                lmd = i;
                for(auto a : t_node.anc)
                {
                    if(tmp_lmds.count(a))
                        break;
                    else
                        tmp_lmds[a] = i;
                }
            }
            else
            {
                lmd = tmp_lmds[t_node.nid];
            }

            lmds.push_back(lmd);
            tmp_keyroots[lmd] = i;
            i++;
        }

        for(auto it = tmp_keyroots.begin(); it!= tmp_keyroots.end(); it++)
        {
            keyroots.push_back(it->second);
        }
        std::sort(keyroots.begin(), keyroots.end());
    }



public:
    Node<D>* root;
    std::vector<Node<D>*> nodes;
    std::vector<int> ids;
    std::vector<int> lmds;
    std::vector<int> keyroots;
};


    
} // namespace xwb
