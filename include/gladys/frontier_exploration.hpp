/*
 * frontier_exploration.hpp
 *
 * Graph Library for Autonomous and Dynamic Systems
 * This class aims at providing a list of frontiers based on the cuurent
 * knowledge of the environment to perform exploration.
 *
 * author:  Cyril Robin <cyril.robin@laas.fr>
 * created: 2013-09-06
 * license: BSD
 */
#ifndef FRONTIER_EXPLORATION_HPP
#define FRONTIER_EXPLORATION_HPP

#include <vector>
#include <ostream>

#include "gladys/point.hpp"
#include "gladys/nav_graph.hpp"

// NOTE : it currently work only with 2D points

namespace gladys {

/* f_atttributes class
 ****************************************************************************/
class f_attributes {

public:
    /* the frontier attributes */
    // NB: the attributes of a frontier are dependent from others' attributes…
    size_t ID;                  // ID of the frontier (= its position in the vector)
    double size;                // size of the frontiers (meters)
    double ratio ;              // importance of the frontier among others
                                // ( max = 1  ; "value < 0" <=> unknown)
    point_xy_t lookout ;        // point from wich we want to observe the frontier
    double distance ;           // euclidian distance to the lookout
    double yaw_diff;            // yaw difference to the lookout
    path_t path ;               // path to the lookout, from the robot
    double cost ;               // the cost of this path
    unsigned int proximity ;    // proximity is the number of robot closer to
                                // the look-out than the robot which computed
                                // the frontier
};

std::ostream& operator<< (std::ostream &out, const f_attributes& f) {
    out << "{ #" << f.ID << ": size = " << f.size << "; ratio = " << f.ratio 
        << "; lookout = (" << f.lookout[0] << "," << f.lookout[1] 
        << "); euclidian distance = " << f.distance
        << "; yaw difference = " << f.yaw_diff
        << "; path size = " << f.path.size() << "; cost = " << f.cost
        << "; proximity = " << f.proximity 
        << " }";
    return out;
}

/* frontier_detector class
 ****************************************************************************/

/* frontier_detector class */
class frontier_detector {

private:
    /* internal data */
    const nav_graph& ng ;           // use for its weight_map and
                                    // the path planning
    const weight_map& map ;         // the weightmap linked with the nav graph
    const gdalwrap::raster& data ;  // the gdal raster of the weightmap

    std::vector< points_t > frontiers ;         // the list of the frontiers
    std::vector< f_attributes > attributes ;    // the frontiers attributes

    // area to explore (generally smaller than the whole weightmap)
    double x0_area ;            // x origin of the area to explore (dtm frame)
    double y0_area ;            // y origin of the area to explore (dtm frame)
    double height_max ;         // height of the area to explore (dtm scale)
    double width_max ;          // width of the area to explore (dtm scale)

    // miscellaneuos parameters used to cnstruct "valid" frontiers
    size_t max_nf   ;           // Max number of frontiers to consider
    double frontier_min_size ;  // Min size of each frontier
    double frontier_max_size ;  // Max size of each frontier
    double min_dist ;           // Min distance to each frontier
    double max_dist ;           // Max distance to each frontier

    // internal parameters
    // x/y min/max define the focused bounded area.
    double x_min ; 
    double x_max ;
    double y_min ; 
    double y_max ;

    /* hidden computing functions */
    /** compute_frontiers_WFD
     *
     * Compute the frontiers with the classical WFD (Wavefront Frontier
     * Detector) algorithm. Result is stored in frontiers.
     *
     * @param seed : the seed for the wavefront detection (usually it is the
     * robot position) ; Note that the seed must be in the "known" area.
     *
     * @throws : throw an exception if the seed is not valid.
     *
     */
    void compute_frontiers_WFD( const point_xy_t &seed );

    /** filter frontiers
     *
     * Quickly compute some attributes to discard non-"valuable" frontiers.
     * Aims at reducing the number of frontier by keeping only the most
     * promising ones, and discarding the others. (This speeds up the
     * computation of costly attributes, and eases the planning beyond.)
     *
     */
    void filter_frontiers(  const points_t& r_pos );

    /** compute_attributes
     *
     * Compute the frontier attributes for each elements in the frontiers list
     * frontiers.
     *
     * @param r_pos : the position of all the robot in the team. The first one
     * is assume to be the robot running the algorithm.
     *
     */
    void compute_attributes( const points_t &r_pos, double yaw ) ;

    /** is_frontier
     *
     * Tell if the given point is a frontier point.
     *
     * @param p : the point which is tested.
     *
     */
    bool is_frontier(  const point_xy_t &p ) ;

    /** find_neighbours()
     *
     * Return the list of adjacent points (or neighbours) of p
     *
     * @param p : the point we focus.
     *
     * @param height : the height of the map.
     *
     * @param width : the height of the map.
     * 
     */
     points_t find_neighbours( const point_xy_t &p ); 

public:
    /* Name of the available algorithms to compute frontiers */
    typedef enum {WFD, FFD} algo_t;

    /** frontier_detector constructor
     *
     * @param nav_graph graph
     */
    frontier_detector( const nav_graph& _ng,
                       double _x0_area,
                       double _y0_area,
                       double _height_max,
                       double _width_max ) :
            ng (_ng),
            map ( ng.get_map() ),
            data ( map.get_weight_band() )



    {
        x0_area    = _x0_area     ;
        y0_area    = _y0_area     ;
        height_max = _height_max  ;
        width_max  = _width_max   ;
    }

    /* public computing functions */
    /** compute_frontiers
     *
     * Compute the frontiers with the given algorithm and parameters.
     * Compute their attributes.
     *
     * @param r_pos : the position of all the robot in the team. The first one
     * is assume to be the robot running the algorithm.
     *
     * @param max_nf : max number of frontiers to consider (other are ignored ;
     * the filter uses some heuristics) ; default is 10
     * is WFD ( Wavefront Frontier Detection).
     *
     * @param frontier_min_size : minimal size of the frontier (other are ignored) ;
     * default is 2.
     *
     * @param frontier_max_size : maximal size of the frontier (other are ignored) ;
     * default is 30.0.
     *
     * @param algo : chose the algorithm used to compute the frontiers ; default
     * is WFD ( Wavefront Frontier Detection).
     *
     * @throws : throw an exception if the algo provided is invalid.
     *
     */
    void compute_frontiers( const points_t &r_pos, double yaw,
        size_t max_nf = 50, double frontier_min_size = 2.0,
        double frontier_max_size = 30.0, double min_dist = 1.6, 
        double max_dist = 50.0, algo_t algo = WFD );

    /* getters */
    const nav_graph& get_graph() const {
        return ng;
    }
    const std::vector< points_t >& get_frontiers() const {
        return frontiers;
    }
    const std::vector< f_attributes >& get_attributes() const {
        return attributes;
    }

};

} // namespace gladys

#endif // FRONTIER_EXPLORATION_HPP

