// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_UGGRID_HH
#define DUNE_UGGRID_HH

/** \file
 * \brief The UGGrid class
 */

#include <dune/common/capabilities.hh>
#include <dune/grid/common/grid.hh>
#include <dune/common/misc.hh>

/* The following lines including the necessary UG headers are somewhat
   tricky.  Here's what's happening:
   UG can support two- and three-dimensional grids.  You choose be setting
   either _2 oder _3 while compiling.  This changes all sorts of stuff, in
   particular data structures in the headers.
   UG was never supposed to provide 2d and 3d grids at the same time.
   However, when compiling it as c++, the dimension-dependent parts are
   wrapped up cleanly in the namespaces UG2d and UG3d, respectively.  That
   way it is possible to link together the UG lib for 2d and the one for 3d.
   But we also need the headers twice!  Once with _2 set and once with _3!
   So here we go:*/

/** \todo Defining __PC__ here is certainly not the perfect way... */
/* We're still forced to include compiler.h, which expects the architecture
   as a macro.  This sucks, but currently there's no better way.  We
   choose __PC__ and hope for the best. */
#define __PC__
/* The following define tells the UG headers that we want access to a few
   special fields, for example the extra index fields in the element data structures. */
#define FOR_DUNE

// Set UG's space-dimension flag to 2d
#define _2
// And include all necessary UG headers
#include "uggrid/ugincludes.hh"

// Wrap a few large UG macros by functions before they get undef'ed away.
// Here: The 2d-version of the macros
#include "uggrid/ugfunctions.hh"

// UG defines a whole load of preprocessor macros.  ug_undef.hh undefines
// them all, so we don't get name clashes.
#include "uggrid/ug_undefs.hh"
#undef _2

/* Now we're done with 2d, and we can do the whole thing over again for 3d */

/* All macros set by UG have been unset.  This includes the macros that ensure
   single inclusion of headers.  We can thus include them again.  However, we
   only want to include those headers that contain dimension-dependent stuff.
   Therefore, we set a few single-inclusion defines manually before include
   ugincludes.hh again.
 */
//#define __COMPILER__
#define __HEAPS__
#define __UGENV__
#define __PARGM_H__
#define __DEVICESH__
#define __SM__

#define _3
#include "uggrid/ugincludes.hh"

// Wrap a few large UG macros by functions before they get undef'ed away.
// This time it's the 3d-versions.
#include "uggrid/ugfunctions3d.hh"

// undef all macros defined by UG
#include "uggrid/ug_undefs.hh"

#undef __PC__
#undef FOR_DUNE

/** \todo Remove this once getChildrenOfSubface is gone */
#include <dune/common/array.hh>

// The components of the UGGrid interface
#include "uggrid/uggridgeometry.hh"
#include "uggrid/uggridentity.hh"
#include "uggrid/uggridentitypointer.hh"
#include "uggrid/uggridboundent.hh"
#include "uggrid/ugintersectionit.hh"
#include "uggrid/uggridleveliterator.hh"
#include "uggrid/uggridleafiterator.hh"
#include "uggrid/uggridhieriterator.hh"
#include "uggrid/uggridindexsets.hh"

namespace Dune {

  //**********************************************************************
  //
  // --UGGrid
  //
  //**********************************************************************

  /**
     \brief [<em> provides \ref Dune::Grid </em>]
     \brief Provides the meshes of the finite element toolbox UG.
     \brief (http://cox.iwr.uni-heidelberg.de/~ug).
     \ingroup GridInterface

     This is the implementation of the grid interface
     using the UG grid management system.  UG provides conforming grids
     in two and three space dimensions.  The grids can be mixed, i.e.
     2d grids can contain triangles and quadrilaterals and 3d grid can
     contain tetrahedra and hexahedra and also pyramids and prisms.
     The grid refinement rules are very flexible.  Local adaptive red/green
     refinement is the default, but a special method in the UGGrid class
     allows you to directly access a number of anisotropic refinements rules.
     Last but not least, the UG grid manager is completely parallelized.

     To use this module you need the UG library.  You can obtain it at
     http://cox.iwr.uni-heidelberg.de/~ug .  Unfortunately, the version
     of UG available for download (3.8 at the time of writing) is not new
     enough to be compatible
     with DUNE.  Please write to one of the contact addresses given on
     the page mentioned above and ask for a more recent version of UG.

     UG is built using the standard GNU autotools.  After unpacking
     the tarball, please type
     \verbatim
      ./configure --prefix=my_ug_install_dir CC=g++-3.4
     \endverbatim
     Note that you need to force the build system to compile UG
     as C++, which is a non-default behaviour.  Also, make sure
     that you're compiling it with a compiler that's binary compatible
     to the one you use for DUNE.

     After that it's simply
     \verbatim
      make
      make install
     \endverbatim
     and you're done.

     After compiling UG you must tell %Dune where to find UG, and which
     grid dimension to use:
     \verbatim
     ./autogen.sh [OPTIONS] --with-ug=PATH_TO_UG --with-problem-dim=DIM --with-world-dim=DIMWORLD
     \endverbatim

     As UG only supports 2d-grids in a 2d-world and 3d-grids in a 3d-world,
     whatever you choose for DIM and DIMWORLD in the line above must be equal
     and either 2 or 3.  The two dimension-related arguments must both be
     there because they actually refer to all external grid implementations
     that may require it, and there are some that do not necessarily pose
     restrictions as strict as UG does.

     In your DUNE application, you can only instantiate UGGrid<2,2> or
     UGGrid<3,3>, depending on what you chose above.

     Please send any questions, suggestions, or bug reports to
     sander@math.fu-berlin.de
   */
  template <int dim, int dimworld>
  class UGGrid : public GridDefault  <dim, dimworld, double, UGGrid<dim,dimworld> >
  {

    friend class UGGridEntity <0,dim,UGGrid<dim,dimworld> >;
    friend class UGGridEntity <dim,dim,UGGrid<dim,dimworld> >;
    friend class UGGridHierarchicIterator<UGGrid<dim,dimworld> >;
    friend class UGGridIntersectionIterator<UGGrid<dim,dimworld> >;

    friend class UGGridLevelIndexSet<dim>;
    friend class UGGridLeafIndexSet<dim>;
    friend class UGGridGlobalIdSet<dim>;
    friend class UGGridLocalIdSet<dim>;

    template<int codim_, int dim_, class GridImp_, template<int,int,class> class EntityImp_>
    friend class Entity;

    /** \brief UGGrid is only implemented for 2 and 3 dimension */
    CompileTimeChecker< (dimworld==dim) && ((dim==2) || (dim==3)) >   Use_UGGrid_only_for_2d_and_3d;

    //**********************************************************
    // The Interface Methods
    //**********************************************************
  public:
    /** \brief The standard Dune grid traits */
    typedef GridTraits<dim,
        dimworld,
        Dune::UGGrid<dim,dimworld> ,
        UGGridGeometry,
        UGGridEntity,
        UGGridBoundaryEntity,
        UGGridEntityPointer,
        UGGridLevelIterator,
        UGGridIntersectionIterator,
        UGGridHierarchicIterator,
        UGGridLeafIterator> Traits;

    typedef UGGridLevelIndexSet<dim> LevelIndexSet;
    typedef UGGridLeafIndexSet<dim>  LeafIndexSet;
    typedef UGGridGlobalIdSet<dim>   GlobalIdSet;
    typedef UGGridLocalIdSet<dim>    LocalIdSet;

    //! The type used to store coordinates
    typedef double ctype;

    /** \brief Constructor with control over UG's memory requirements
     *
     * \param heapSize The size of UG's internal memory in megabytes.  UG allocates
     * memory only once.  I don't know what happens if you create UGGrids with
     * differing heap sizes.
     * \param envHeapSize The size of UG's environment heap, also in megabytes.
     */
    UGGrid(unsigned int heapSize, unsigned int envHeapSize);

    /** \brief Constructor with default memory settings
     *
     * The default values are 500MB for the general heap and 10MB for
     * the environment heap.
     */
    UGGrid();

    //! Desctructor
    ~UGGrid();

    //! Return maximum level defined in this grid. Levels are numbered
    //! 0 ... maxlevel with 0 the coarsest level.
    int maxlevel() const;

    //! Iterator to first entity of given codim on level
    template<int codim>
    typename Traits::template Codim<codim>::LevelIterator lbegin (int level) const;

    //! one past the end on this level
    template<int codim>
    typename Traits::template Codim<codim>::LevelIterator lend (int level) const;

    //! Iterator to first entity of given codim on level
    template<int codim, PartitionIteratorType PiType>
    typename Traits::template Codim<codim>::template Partition<PiType>::LevelIterator lbegin (int level) const;

    //! one past the end on this level
    template<int codim, PartitionIteratorType PiType>
    typename Traits::template Codim<codim>::template Partition<PiType>::LevelIterator lend (int level) const;

    //! Iterator to first entity of given codim on level
    template<int codim>
    typename Traits::template Codim<codim>::LeafIterator leafbegin() const {
      return typename Traits::template Codim<codim>::template Partition<All_Partition>::LeafIterator(*this);
    }

    //! one past the end on this level
    template<int codim>
    typename Traits::template Codim<codim>::LeafIterator leafend() const {
      return UGGridLeafIterator<codim,All_Partition, const UGGrid<dim,dimworld> >();
    }

    //! Iterator to first entity of given codim on level
    template<int codim, PartitionIteratorType PiType>
    typename Traits::template Codim<codim>::template Partition<PiType>::LeafIterator leafbegin() const {
      return typename Traits::template Codim<codim>::template Partition<PiType>::LeafIterator(*this);
    }

    //! one past the end on this level
    template<int codim, PartitionIteratorType PiType>
    typename Traits::template Codim<codim>::template Partition<PiType>::LeafIterator leafend() const {
      return UGGridLeafIterator<codim,All_Partition, const UGGrid<dim,dimworld> >();
    }

    /** \brief Number of grid entities per level and codim
     */
    int size (int level, int codim) const;

    //! number of leaf entities per codim in this process
    int size (int codim) const
    {
      DUNE_THROW(NotImplemented, "not implemented");
      return 0;
    }

    //! number of entities per level, codim and geometry type in this process
    int size (int level, int codim, GeometryType type) const
    {
      DUNE_THROW(NotImplemented, "not implemented");
      return 0;
    }

    //! number of leaf entities per codim and geometry type in this process
    int size (int codim, GeometryType type) const
    {
      DUNE_THROW(NotImplemented, "not implemented");
      return 0;
    }

    const GlobalIdSet& globalidset() const
    {
      return globalIdSet_;
    }

    const LocalIdSet& localidset() const
    {
      return localIdSet_;
    }

    const LevelIndexSet& levelindexset(int level) const
    {
      return levelIndexSets_[level];
    }

    const LeafIndexSet& leafindexset() const
    {
      return leafIndexSet_;
    }


    /** \brief Mark entity for refinement
     *
     * This only works for entities of codim 0.
     * The parameter is currently ignored
     *
     * \return <ul>
     * <li> true, if element was marked </li>
     * <li> false, if nothing changed </li>
     * </ul>
     */
    bool mark(int refCount, typename Traits::template Codim<0>::EntityPointer & e );

    /** \brief Mark method accepting a UG refinement rule
     */
    bool mark(typename Traits::template Codim<0>::EntityPointer & e, typename UG_NS<dim>::RefinementRule rule);

    //! Triggers the grid refinement process
    bool adapt();

    /** \brief Clean up refinement markers */
    void postAdapt();

    /** \brief Please doc me! */
    GridIdentifier type () const { return UGGrid_Id; };

    /** \brief Distributes this grid over the available nodes in a distributed machine
     *
       \param minlevel The coarsest grid level that gets distributed
       \param maxlevel does currently get ignored
     */
    void loadBalance(int strategy, int minlevel, int depth, int maxlevel, int minelement);

    /** \brief The communication interface
       @param T: array class holding data associated with the entities
       @param P: type used to gather/scatter data in and out of the message buffer
       @param codim: communicate entites of given codim
       @param if: one of the predifined interface types, throws error if it is not implemented
       @param level: communicate for entities on the given level

       Implements a generic communication function sending an object of type P for each entity
       in the intersection of two processors. P has two methods gather and scatter that implement
       the protocol. Therefore P is called the "protocol class".
     */
    template<class T, template<class> class P, int codim>
    void communicate (T& t, InterfaceType iftype, CommunicationDirection dir, int level);

    // **********************************************************
    // End of Interface Methods
    // **********************************************************

    /** \brief Start the coarse grid creation process */
    void createbegin();

    /** \brief End the coarse grid creation process */
    void createend();

    /** \brief Adapt the grid without constructing the green closure

       WARNING: This is a very special method.  Omitting the green closure does
       not mean that UG creates correct nonconforming meshes.  For internal
       reasons (bugs?) though, it allows you to do uniform refinement with
       a few anisotropic refinement rules such as UG3d::PRISM_QUADSECT or
       UG3d::HEX_QUADSECT_0.
     */
    void adaptWithoutClosure();

    /** \brief Rudimentary substitute for a hierarchic iterator on faces
        \param e, elementSide Grid face specified by an element and one of its sides
        \param maxl The finest level that should be traversed by the iterator
        \param children For each subface: element index, elementSide, and level
     */
    void getChildrenOfSubface(typename Traits::template Codim<0>::EntityPointer & e,
                              int elementSide,
                              int maxl,
                              Array<typename Dune::UGGridEntityPointer<0,UGGrid> >& childElements,
                              Array<unsigned char>& childElementSides) const;

    /** \brief The different forms of grid refinement that UG supports */
    enum RefinementType {
      /** \brief New level consists only of the refined elements */
      LOCAL,
      /** \brief New level consists of the refined elements and the unrefined ones, too */
      COPY
    };

    /** \brief Sets the type of grid refinement */
    void setRefinementType(RefinementType type) {
      refinementType_ = type;
    }

    /** \brief Collapses the grid hierarchy into a single grid */
    void collapse() {
      if (Collapse(multigrid_))
        DUNE_THROW(GridError, "UG" << dim << "d::Collapse() returned error code!");
    }

    /** \brief Read access to the UG-internal grid name */
    const std::string& name() const {return name_;}

    /** \brief Calls a few interal methods to properly set up a UG grid */
    void makeNewUGMultigrid();

    /** \brief Does uniform refinement
     *
     * \param n Number of uniform refinement steps
     */
    void globalRefine(int n);

    // UG multigrid, which contains the data
    typename UGTypes<dimworld>::MultiGridType* multigrid_;

  public:
    // I need this to store some information that gets passed
    // to the boundary description
    void* extra_boundary_data_;

  private:

    // Access to entity implementations through the interface wrappers
    template <int cd>
    UGGridEntity<cd,dim,const UGGrid>& getRealEntity(typename Traits::template Codim<cd>::Entity& entity) {
      return entity.realEntity;
    }

    // Const access to entity implementations through the interface wrappers
    template <int cd>
    const UGGridEntity<cd,dim,const UGGrid>& getRealEntity(const typename Traits::template Codim<cd>::Entity& entity) const {
      return entity.realEntity;
    }

    // Start up the UG system
    void init(unsigned int heapSize, unsigned int envHeapSize);

    // Recomputes entity indices after the grid was changed
    void setIndices();

    // Each UGGrid object has a unique name to identify it in the
    // UG environment structure
    std::string name_;

    // Our set of level indices
    std::vector<LevelIndexSet> levelIndexSets_;

    LeafIndexSet leafIndexSet_;

    GlobalIdSet globalIdSet_;

    LocalIdSet localIdSet_;

    //! Marks whether the UG environment heap size is taken from
    //! an existing defaults file or whether the values from
    //! the UGGrid constructor are taken
    static bool useExistingDefaultsFile;

    //! The type of grid refinement currently in use
    RefinementType refinementType_;

    //!
    bool omitGreenClosure_;

    /** \brief Number of UGGrids currently in use.
     *
     * This counts the number of UGGrids currently instantiated.  All
     * constructors of UGGrid look at this variable.  If it zero, they
     * initialize UG before proceeding.  Destructors use the same mechanism
     * to safely shut down UG after deleting the last UGGrid object.
     */
    static int numOfUGGrids;

    /** \brief The arguments to UG's newformat command
     *
     * They need to be allocated dynamically, because UG writes into
     * some of them.  That causes the code to crash if it has been
     * compiled with gcc.
     */
    char* newformatArgs[4];

    /** \brief The size of UG's internal heap in megabytes
     *
     * It is handed over to UG for each new multigrid.  I don't know
     * what happens if you hand over differing values.
     */
    unsigned int heapsize;


  }; // end Class UGGrid

  namespace Capabilities
  {

    template<int dim,int dimw>
    struct hasLeafIterator< UGGrid<dim,dimw> >
    {
      static const bool v = true;
    };

    template<int dim, int dimw>
    struct hasEntity< UGGrid<dim,dimw>, 0>
    {
      static const bool v = true;
    };

    template<int dim, int dimw>
    struct hasEntity< UGGrid<dim,dimw>, dim>
    {
      static const bool v = true;
    };

    template<int dim,int dimw>
    struct isParallel< UGGrid<dim,dimw> >
    {
      static const bool v = true;
    };

  }

} // namespace Dune

#endif
