/* 
 * File:   pather_optimizer_fastgraph.h
 * Author: Dev
 *
 * Created on October 9, 2012, 11:28 AM
 */

#ifndef MGL_PATHER_OPTIMIZER_FASTGRAPH_H
#define	MGL_PATHER_OPTIMIZER_FASTGRAPH_H

#include "pather_optimizer.h"
#include "simple_topology.h"
#include "basic_boxlist.h"
#include "intersection_index.h"
#include "Exception.h"
#include "configuration.h"
#include <iostream>
#include <list>

namespace mgl {

class GraphException : public Exception {
public:
    template <typename T>
    GraphException(const T& arg) : Exception(arg) {}
};

/* 
 Implementation of interface put forth in abstract_optimizer
   One of the several different options for optimizing paths. 
    Usage:
    pather_optimizer_fastgraph optimus(grueCfg);
    optimus.addBoundary(loop1);
    optimus.addBoundary(loop2); //all boundaries must be added first
    optimus.addPaths(listOfLoops, somelabel);
    optimus.addPaths(listOfPaths, somedifferentlabel);
    optimus.optimize(destinationlistOfLabeledPaths);    //this is destructive
 */
class pather_optimizer_fastgraph : public abstract_optimizer {
public:
    pather_optimizer_fastgraph(const GrueConfig& grueConf)
            : grueCfg(grueConf), historyPoint(std::numeric_limits<Scalar>::min(), 
            std::numeric_limits<Scalar>::min()) {}
    //addPath builds up the correct interior graph (the correct bucket)
    void addPath(const OpenPath& path, const PathLabel& label);
    void addPath(const Loop& loop, const PathLabel& label);
    //Do not cross this path!
    void addBoundary(const OpenPath& path);
    //Creates a new bucket. Things inside of this loop will be added to this bucket
	void addBoundary(const Loop& loop);
    void clearBoundaries();
	void clearPaths();
    //debugging: Make a nice svg of this graph
    void repr_svg(std::ostream& out);
protected:
    void optimizeInternal(LabeledOpenPaths& labeledpaths);
private:
    typedef std::list<LabeledOpenPaths> multipath_type;
    
    class bucket;
    typedef std::list<bucket> bucket_list;
    //format is void foo(output, [input]);
    //pick the best bucket to start optimizing, optimize it, repeat until done
    void optimize1(multipath_type& output, PointType& entryPoint);
    //optimize the bucket selected by above
    void optimize1Inner(LabeledOpenPaths& labeledpaths, bucket_list::iterator input, 
            PointType& entryPoint);
    //Run v-opt on results of above function
    bool optimize2(LabeledOpenPaths& labeledopenpaths, 
            LabeledOpenPaths& intermediate);
    class Cost : public PathLabel {
    public:
        Cost(const PathLabel& label = PathLabel(), 
                Scalar distance = 0, 
                PointType normal = PointType()) 
                : PathLabel(label), m_distance(distance), m_normal(normal) {}
        Cost(const Cost& other) : PathLabel(other), 
                m_distance(other.m_distance), m_normal(other.m_normal) {}
        Scalar distance() const { return m_distance; }
        const PointType& normal() const { return m_normal; }
    private:
        Scalar m_distance;
        PointType m_normal;
    };
    class NodeData {
    public:
        NodeData(PointType position, 
                int priority, 
                bool entry) 
                : m_position(position), 
                m_priority(priority), 
                m_isentry(entry) {}
        NodeData() : m_priority(0), m_isentry(false) {}
        const PointType& getPosition() const { return m_position; }
        int getPriority() const { return m_priority; }
        bool isEntry() const { return m_isentry; }
    private:
        PointType m_position;
        int m_priority;
        bool m_isentry;
    };
    
    typedef basic_boxlist<libthing::LineSegment2> boundary_container;
    typedef topo::simple_graph<NodeData, Cost> graph_type;
    typedef graph_type::node node;
    typedef graph_type::node_index node_index;
    typedef std::pair<node_index, Scalar> probe_link_type;
    
    class bucket {
    public:
        boundary_container m_bounds;
        graph_type m_graph;
    };
    
    typedef graph_type::forward_node_iterator node_iterator;
    
    class entry_iterator {
    public:
        
        friend class pather_optimizer_fastgraph;
        
        entry_iterator() {}
            
        entry_iterator& operator ++(); //pre
        entry_iterator operator ++(int); //post
        node& operator *();
        node* operator ->() { return &**this; }
        bool operator ==(const entry_iterator& other) const;
        bool operator !=(const entry_iterator& other) const
                { return !(*this==other); }
        
    private:
        explicit entry_iterator(node_iterator base, node_iterator end) 
                : m_base(base), m_end(end) {}
        node_iterator m_base;
        node_iterator m_end;
    };
    
    entry_iterator entryBegin(graph_type& graph);
    entry_iterator entryEnd(graph_type& graph);
    
    class probeCompare {
    public:
        probeCompare(node_index from, graph_type& basis) 
                : m_from(from), m_graph(basis) {}
        bool operator ()(const probe_link_type& lhs, 
                const probe_link_type& rhs);
    private:
        node_index m_from;
        graph_type& m_graph;
    };
    
    node::forward_link_iterator bestLink(node& from, graph_type& graph, 
            boundary_container& boundaries); //can return node::forwardEnd()
    void buildLinks(node& from, graph_type& graph, 
            boundary_container& boundaries);
    
    static bool compareConnections(const node::connection& lhs, 
            const node::connection& rhs);
    static bool compareNodes(const node& lhs, const node& rhs);
    static bool comparePathLists(const LabeledOpenPaths& lhs, 
            const LabeledOpenPaths& rhs);
    static size_t countIntersections(libthing::LineSegment2& line, 
            boundary_container& boundContainer);
    
    class connectionComparator {
    public:
        connectionComparator(PointType unit = PointType()) : m_unit(unit) {}
        bool operator ()(const node::connection& lhs, 
                const node::connection& rhs) const;
    private:
        PointType m_unit;
    };
    
    class nodeComparator {
    public:
        nodeComparator(graph_type& graph, PointType point = 
                PointType(std::numeric_limits<Scalar>::min(), 
                std::numeric_limits<Scalar>::min()))
                : m_graph(graph), m_position(point) {}
        bool operator ()(const node& lhs, const node& rhs) const;
        bool operator ()(node_index lhs, node_index rhs) const;
    private:
        graph_type& m_graph;
        PointType m_position;
    };
    
    class bucketSorter {
    public:
        bucketSorter(boundary_container& bounds, PointType frompoint) 
                : m_bounds(bounds), m_from(frompoint) {}
        bool operator ()(const bucket& lhs, const bucket& rhs) const;
    private:
        boundary_container& m_bounds;
        PointType m_from;
    };
    
    bool crossesBounds(const libthing::LineSegment2& line, 
            boundary_container& boundaries);
    
    void smartAppendPoint(PointType point, PathLabel label, 
            LabeledOpenPaths& labeledpaths, LabeledOpenPath& path, 
            PointType& entryPoint);
    void smartAppendPath(LabeledOpenPaths& labeledpaths, LabeledOpenPath& path);
    
    Scalar splitPaths(multipath_type& destionation, const LabeledOpenPaths& source);
    bucket_list::iterator pickBucket(PointType point);
    
    
    const GrueConfig& grueCfg;
    
    boundary_container m_boundaries;
    bucket_list buckets;
    graph_type m_graph;
    AABBox boundaryLimits;
    PointType historyPoint;
    
    
};

}

#endif	/* MGL_PATHER_OPTIMIZER_FASTGRAPH_H */
