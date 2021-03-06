#ifndef DUNE_GRID_UTILITY_PARMETISGRIDPARTITIONER_HH
#define DUNE_GRID_UTILITY_PARMETISGRIDPARTITIONER_HH

/** \file
 *  \brief Compute a repartitioning of a Dune grid using ParMetis
 */

#include <algorithm>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/exceptions.hh>

#include <dune/geometry/referenceelements.hh>

#include <dune/grid/utility/globalindexset.hh>
#include <dune/grid/common/mcmgmapper.hh>

#if HAVE_PARMETIS
#include <parmetis.h>

namespace Dune
{

  /** \brief Compute a repartitioning of a Dune grid using ParMetis
   *
   * \author Benjamin Bykowski
   */
  template<class GridView>
  struct ParMetisGridPartitioner {
#if PARMETIS_MAJOR_VERSION < 4
    typedef idxtype idx_t;
    typedef float real_t;
#endif

    typedef typename GridView::template Codim<0>::Iterator                                         ElementIterator;
    typedef typename GridView::template Codim<0>::template Partition<Interior_Partition>::Iterator InteriorElementIterator;
    typedef typename GridView::IntersectionIterator                                                IntersectionIterator;

    enum {
      dimension = GridView::dimension
    };


    /** \brief Create an initial partitioning of a Dune grid, i.e., not taking into account communication cost
     *
     * This pipes a Dune grid into the method ParMETIS_V3_PartMeshKway (see the ParMetis documentation for details)
     *
     * \param gv The grid view to be partitioned
     * \param mpihelper The MPIHelper object, needed to get the MPI communicator
     *
     * \return std::vector with one uint per All_Partition element.  For each element, the entry is the
     *    number of the partition the element is assigned to.
     */
    static std::vector<unsigned> partition(const GridView& gv, const Dune::MPIHelper& mpihelper) {
      const unsigned numElements = gv.size(0);

      std::vector<unsigned> part(numElements);

      // Setup parameters for ParMETIS
      idx_t wgtflag = 0;                                  // we don't use weights
      idx_t numflag = 0;                                  // we are using C-style arrays
      idx_t ncon = 1;                                     // number of balance constraints
      idx_t ncommonnodes = 2;                             // number of nodes elements must have in common to be considered adjacent to each other
      idx_t options[4] = {0, 0, 0, 0};                    // use default values for random seed, output and coupling
      idx_t edgecut;                                      // will store number of edges cut by partition
      idx_t nparts = mpihelper.size();                    // number of parts equals number of processes
      std::vector<real_t> tpwgts(ncon*nparts, 1./nparts); // load per subdomain and weight (same load on every process)
      std::vector<real_t> ubvec(ncon, 1.05);              // weight tolerance (same weight tolerance for every weight there is)

      // The difference elmdist[i+1] - elmdist[i] is the number of nodes that are on process i
      std::vector<idx_t> elmdist(nparts+1);
      elmdist[0] = 0;
      std::fill(elmdist.begin()+1, elmdist.end(), gv.size(0)); // all elements are on process zero

      // Create and fill arrays "eptr", where eptr[i] is the number of vertices that belong to the i-th element, and
      // "eind" contains the vertex-numbers of the i-the element in eind[eptr[i]] to eind[eptr[i+1]-1]
      std::vector<idx_t> eptr, eind;
      int numVertices = 0;
      eptr.push_back(numVertices);

      for (InteriorElementIterator eIt = gv.template begin<0, Interior_Partition>(); eIt != gv.template end<0, Interior_Partition>(); ++eIt) {
        const int curNumVertices = ReferenceElements<double, dimension>::general(eIt->type()).size(dimension);

        numVertices += curNumVertices;
        eptr.push_back(numVertices);

        for (size_t k = 0; k < curNumVertices; ++k)
          eind.push_back(gv.indexSet().subIndex(*eIt, k, dimension));
      }

      // Partition mesh using ParMETIS
      if (0 == mpihelper.rank()) {
        MPI_Comm comm = Dune::MPIHelper::getLocalCommunicator();

#if PARMETIS_MAJOR_VERSION >= 4
        const int OK =
#endif
        ParMETIS_V3_PartMeshKway(elmdist.data(), eptr.data(), eind.data(), NULL, &wgtflag, &numflag,
                                 &ncon, &ncommonnodes, &nparts, tpwgts.data(), ubvec.data(),
                                 options, &edgecut, reinterpret_cast<idx_t*>(part.data()), &comm);

#if PARMETIS_MAJOR_VERSION >= 4
        if (OK != METIS_OK)
          DUNE_THROW(Dune::Exception, "ParMETIS returned an error code.");
#endif
      }

      return part;
    }

    /** \brief Create a repartitioning of a distributed Dune grid
     *
     * This pipes a Dune grid into the method ParMETIS_V3_AdaptiveRepart (see the ParMetis documentation for details)
     *
     * \param gv The grid view to be partitioned
     * \param mpihelper The MPIHelper object, needed to get the MPI communicator
     * \param itr ParMetis parameter to balance grid transport cost vs. communication cost for the redistributed grid.
     *
     * \return std::vector with one uint per All_Partition element.  For each Interior_Partition element, the entry is the
     *    number of the partition the element is assigned to.
     */
    static std::vector<unsigned> repartition(const GridView& gv, const Dune::MPIHelper& mpihelper, real_t& itr = 1000) {

      // Create global index map
      GlobalIndexSet<GridView> globalIndex(gv,0);

      int numElements = std::distance(gv.template begin<0, Interior_Partition>(),
                                                 gv.template end<0, Interior_Partition>());

      std::vector<unsigned> interiorPart(numElements);

      // Setup parameters for ParMETIS
      idx_t wgtflag = 0;                                  // we don't use weights
      idx_t numflag = 0;                                  // we are using C-style arrays
      idx_t ncon = 1;                                     // number of balance constraints
      idx_t options[4] = {0, 0, 0, 0};                    // use default values for random seed, output and coupling
      idx_t edgecut;                                      // will store number of edges cut by partition
      idx_t nparts = mpihelper.size();                    // number of parts equals number of processes
      std::vector<real_t> tpwgts(ncon*nparts, 1./nparts); // load per subdomain and weight (same load on every process)
      std::vector<real_t> ubvec(ncon, 1.05);              // weight tolerance (same weight tolerance for every weight there is)

      MPI_Comm comm = Dune::MPIHelper::getCommunicator();

      // Make the number of interior elements of each processor available to all processors
      std::vector<int> offset(gv.comm().size());
      std::fill(offset.begin(), offset.end(), 0);

      gv.comm().template allgather<int>(&numElements, 1, offset.data());

      // The difference vtxdist[i+1] - vtxdist[i] is the number of elements that are on process i
      std::vector<idx_t> vtxdist(gv.comm().size()+1);
      vtxdist[0] = 0;

      for (unsigned int i=1; i<vtxdist.size(); ++i)
        vtxdist[i] = vtxdist[i-1] + offset[i-1];

      // Set up element adjacency lists
      std::vector<idx_t> xadj, adjncy;
      xadj.push_back(0);

      for (InteriorElementIterator eIt = gv.template begin<0, Interior_Partition>(); eIt != gv.template end<0, Interior_Partition>(); ++eIt)
      {
        size_t numNeighbors = 0;

        for (IntersectionIterator iIt = gv.template ibegin(*eIt); iIt != gv.template iend(*eIt); ++iIt) {
          if (iIt->neighbor()) {
            adjncy.push_back(globalIndex.index(*iIt->outside()));

            ++numNeighbors;
          }
        }

        xadj.push_back(xadj.back() + numNeighbors);
      }

#if PARMETIS_MAJOR_VERSION >= 4
      const int OK =
#endif
        ParMETIS_V3_AdaptiveRepart(vtxdist.data(), xadj.data(), adjncy.data(), NULL, NULL, NULL,
                                   &wgtflag, &numflag, &ncon, &nparts, tpwgts.data(), ubvec.data(),
                                   &itr, options, &edgecut, reinterpret_cast<idx_t*>(interiorPart.data()), &comm);

#if PARMETIS_MAJOR_VERSION >= 4
      if (OK != METIS_OK)
        DUNE_THROW(Dune::Exception, "ParMETIS returned error code " << OK);
#endif

      // At this point, interiorPart contains a target rank for each interior element, and they are sorted
      // by the order in which the grid view traverses them.  Now we need to do two things:
      // a) Add additional dummy entries for the ghost elements
      // b) Use the element index for the actual ordering.  Since there may be different types of elements,
      //    we cannot use the index set directly, but have to go through a Mapper.

      typedef MultipleCodimMultipleGeomTypeMapper<GridView, MCMGElementLayout> ElementMapper;
      ElementMapper elementMapper(gv);

      std::vector<unsigned int> part(gv.size(0));
      std::fill(part.begin(), part.end(), 0);
      unsigned int c = 0;
      for (InteriorElementIterator eIt = gv.template begin<0, Interior_Partition>();
           eIt != gv.template end<0, Interior_Partition>();
           ++eIt)
      {
        part[elementMapper.index(*eIt)] = interiorPart[c++];
      }
      return part;
    }
  };

}  // namespace Dune

#else // HAVE_PARMETIS
#warning "PARMETIS was not found, please check your configuration"
#endif

#endif // DUNE_GRID_UTILITY_PARMETISGRIDPARTITIONER_HH
