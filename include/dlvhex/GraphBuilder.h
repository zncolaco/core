/* -*- C++ -*- */

/**
 * @file GraphBuilder.h
 * @author Roman Schindlauer
 * @date Fri Sep  9 14:40:26 CEST 2005
 *
 * @brief Abstract strategy class for building a dependency graph from rules.
 *
 *
 */

#ifndef _GRAPHBUILDER_H
#define _GRAPHBUILDER_H

#include "dlvhex/Component.h"
#include "dlvhex/Rule.h"

/**
 * @brief Abstract strategy class for building a dependency graph from rules.
 *
 * A GraphBuilder creates a dependency graph for a program.
 */
class GraphBuilder
{
public:
    virtual
    ~GraphBuilder()
    { }

    void
    createNodes(Rules& program);

    
    /**
     * @brief Creates nodes and their dependencies from rules.
     */
    virtual void
    build(Program& program, std::vector<Subgraph*>& subgraphs) = 0;

    /**
     * @brief Finds components from a set of nodes.
     */
//    virtual void
//    findComponents(std::vector<Node> &nodes,
//                   std::vector<Component> &components) = 0;


protected:

    std::vector<Node> nodes;

    GraphBuilder()
    { }
};

class SimpleGraphBuilder : public GraphBuilder
{
public:

    SimpleGraphBuilder();

    virtual void
    build(Program& program, std::vector<Subgraph*>& subgraphs);
//    virtual void
//    findComponents(std::vector<Node> &nodes,
//                   std::vector<Component> &components);
};


#endif /* _GRAPHBUILDER_H */
