#pragma once

#include "Node.h"

template<class D>
class CostModel 
{
public:
  virtual float del(Node<D>* n)=0;
  virtual float ins(Node<D>* n)=0;
  virtual float ren(Node<D>* n1, Node<D>* n2)=0;
};
