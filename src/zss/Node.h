#pragma once

#include <vector>

template <class D>
class Node
{
public:
    D* nodeData;
    Node<D>* parent;
    std::vector<Node<D>*> children;
    bool visited;

    Node()
        :nodeData(nullptr), parent(nullptr), visited(false)
    {

    }

    Node(D* d)
        :nodeData(d), parent(nullptr), visited(false)
    {

    }

    ~Node()
    {
        delete nodeData;
        for(auto& node : children)
        {
            delete node;
        }
    }

    int getNodeCount()
    {
        int sum = 1;
        for (auto child : children)
        {
            sum += child->getNodeCount();
        }
        return sum;
    }

    void addChild(Node<D>* c)
    {
        c->parent = this;
        children.push_back(c);
    }

    D* getNodeData()
    {
        return nodeData;
    }

    void setNodeData(D* d)
    {
        nodeData = d;
    }

    std::vector<Node<D>* > getChildren()
    {
        return children;
    }
};
