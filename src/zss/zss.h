#pragma once

#include "DocNode.h"


namespace xwb
{


template<class D>
struct TEDOp
{
    int type;
    Node<D>* arg1;
    Node<D>* arg2;


    TEDOp()
        :type(0), arg1(nullptr), arg2(nullptr)
    {

    }

    TEDOp(int op, Node<D>* a, Node<D>* b)
        :type(op), arg1(a), arg2(b)
    {

    }

    std::string typeStr() const
    {
        return type ? (type == 1 ? "insert" : (type==2? "delete" : "modify")) : "match";
    }
} ;


int zss_ted(Node<DocNode>* A,  Node<DocNode>* B, std::vector<TEDOp<DocNode>>& ops);
int zss_ted_backtrack(Node<DocNode>* A,  Node<DocNode>* B, std::vector<TEDOp<DocNode>>& ops);



} // namespace xwb
