#pragma once

#include <string>
#include "zss/CostModel.h"
#include "Table.h"


namespace xwb
{
    
enum DocNodeType{
    DOC,
    TABLE,
    ROW,
    CELL
};

class DocNode
{
public:
    DocNode()
      :page_index(-1), table_index(-1), row_index(-1), cell(nullptr)
    {
        
    }

    DocNode(DocNodeType wtype)
      :ntype(wtype), page_index(-1), table_index(-1), row_index(-1), cell(nullptr)
    {
        
    }

    std::wstring getLabel();

public:
    DocNodeType ntype;
    int page_index;
    int table_index;
    int row_index;
    std::wstring label;
    TableCell* cell;
};


class DocNodeCostModel : public CostModel<DocNode>
{
public:
  float del(Node<DocNode>* n)
  {
    return 1.0f;
  }

  float ins(Node<DocNode>* n)
  {
    return 1.0f;
  }

  float ren(Node<DocNode>* n1, Node<DocNode>* n2)
  {
    return (n1->getNodeData()->getLabel() == (n2->getNodeData()->getLabel())) ? 0.0f : 1.0f;
  }
};




} // namespace xwb