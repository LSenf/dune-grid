// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_GRID_YASPGRIDINTERSECTION_HH
#define DUNE_GRID_YASPGRIDINTERSECTION_HH

/** \file
 * \brief The YaspIntersection class
 *
   YaspIntersection provides data about intersection with
   neighboring codim 0 entities.
 */

namespace Dune {

  /** \brief  YaspIntersection provides data about intersection with
     neighboring codim 0 entities.
   */
  template<class GridImp>
  class YaspIntersection
  {
    enum { dim=GridImp::dimension };
    enum { dimworld=GridImp::dimensionworld };
    typedef typename GridImp::ctype ctype;
    YaspIntersection();
    YaspIntersection& operator = (const YaspIntersection&);

    typedef typename GridImp::Traits::template Codim< 1 >::GeometryImpl GeometryImpl;
    typedef typename GridImp::Traits::template Codim< 1 >::LocalGeometryImpl LocalGeometryImpl;

    friend class YaspIntersectionIterator<GridImp>;

  public:
    // types used from grids
    typedef typename GridImp::YGridLevelIterator YGLI;
    typedef typename GridImp::YGrid::Iterator I;
    typedef typename GridImp::template Codim<0>::Entity Entity;
    typedef typename GridImp::template Codim<0>::EntityPointer EntityPointer;
    typedef typename GridImp::template Codim<1>::Geometry Geometry;
    typedef typename GridImp::template Codim<1>::LocalGeometry LocalGeometry;

    void update() const {
      if (_count == 2*_dir + _face || _count >= 2*dim)
        return;

      // cleanup old stuff
      _outside.transformingsubiterator().move(_dir,1-2*_face);   // move home

      // update face info
      _dir = _count / 2;
      _face = _count % 2;

      // move transforming iterator
      _outside.transformingsubiterator().move(_dir,-1+2*_face);
    }

    /*! return true if neighbor ist outside the domain. Still the neighbor might
       exist in case of periodic boundary conditions, i.e. true is returned
       if the neighbor is outside the periodic unit cell
     */
    bool boundary () const
    {
      return (_inside.transformingsubiterator().coord(_count/2) + 2*(_count%2) - 1 < 0
              ||
              _inside.transformingsubiterator().coord(_count/2) + 2*(_count%2) - 1 > _inside.gridlevel()->mg->levelSize(_inside.gridlevel()->level(),_count/2)- 1) ;
    }

    //! return true if neighbor across intersection exists in this processor
    bool neighbor () const
    {
      return (_inside.transformingsubiterator().coord(_count/2) + 2*(_count%2) - 1 >= _inside.gridlevel()->overlap[0].dataBegin()->min(_count/2)
              &&
              _inside.transformingsubiterator().coord(_count/2) + 2*(_count%2) - 1 <= _inside.gridlevel()->overlap[0].dataBegin()->max(_count/2));
    }

    //! Yasp is always conform
    bool conforming () const
    {
      return true;
    }

    //! return EntityPointer to the Entity on the inside of this intersection
    //! (that is the Entity where we started this Iterator)
    EntityPointer inside() const
    {
      return _inside;
    }

    //! return EntityPointer to the Entity on the outside of this intersection
    EntityPointer outside() const
    {
      update();
      return _outside;
    }

#if DUNE_GRID_EXPERIMENTAL_GRID_EXTENSIONS
    //! identifier for boundary segment from macro grid
    //! (attach your boundary condition as needed)
    int boundaryId() const
    {
      if(boundary()) return indexInInside()+1;
      return 0;
    }
#endif

    //! identifier for boundary segment from macro grid
    //! (attach your boundary condition as needed)
    int boundarySegmentIndex() const
    {
      if(! boundary())
        DUNE_THROW(GridError, "called boundarySegmentIndex while boundary() == false");
      update();
      // size of local macro grid
      const Dune::array<int, dim> & size = _inside.gridlevel()->mg->begin()->overlap[0].dataBegin()->size();
      const Dune::array<int, dim> & origin = _inside.gridlevel()->mg->begin()->overlap[0].dataBegin()->origin();
      Dune::array<int, dim> sides;
      {
        for (int i=0; i<dim; i++)
        {
          sides[i] =
            ((_inside.gridlevel()->mg->begin()->overlap[0].dataBegin()->origin(i)
              == 0)+
            (_inside.gridlevel()->mg->begin()->overlap[0].dataBegin()->origin(i) +
                      _inside.gridlevel()->mg->begin()->overlap[0].dataBegin()->size(i)
                      ==
                      _inside.gridlevel()->mg->levelSize(0,i)));

        }
      }
      // global position of the cell on macro grid
      Dune::array<int, dim> pos = _inside.transformingsubiterator().coord();
      for(int i=0; i<dim; i++)
      {
        pos[i] = pos[i] / (1<<_inside.level());
        pos[i] = pos[i] - origin[i];
      }
      // compute unit-cube-face-sizes
      Dune::array<int, dim> fsize;
      {
        int vol = 1;
        for (int k=0; k<dim; k++)
          vol *= size[k];
        for (int k=0; k<dim; k++)
          fsize[k] = vol/size[k];
      }
      // compute index in the unit-cube-face
      int index = 0;
      {
        int localoffset = 1;
        for (int k=dim-1; k>=0; k--)
        {
          if (k == _dir) continue;
          index += (pos[k]) * localoffset;
          localoffset *= size[k];
        }
      }
      // add unit-cube-face-offsets
      {
        for (int k=0; k<_dir; k++)
          index += sides[k] * fsize[k];
        // add fsize if we are on the right face and there is a left-face-boundary on this processor
        index += _face * (sides[_dir]>1) * fsize[_dir];
      }

      return index;
    }

    //! return unit outer normal, this should be dependent on local coordinates for higher order boundary
    FieldVector<ctype, dimworld> outerNormal (const FieldVector<ctype, dim-1>& local) const
    {
      return _faceInfo[_count].normal;
    }

    //! return unit outer normal, this should be dependent on local coordinates for higher order boundary
    FieldVector<ctype, dimworld> unitOuterNormal (const FieldVector<ctype, dim-1>& local) const
    {
      return _faceInfo[_count].normal;
    }

    //! return unit outer normal at center of intersection geometry
    FieldVector<ctype, dimworld> centerUnitOuterNormal () const
    {
      return _faceInfo[_count].normal;
    }

    //! return unit outer normal, this should be dependent on
    //! local coordinates for higher order boundary
    //! the normal is scaled with the integration element of the intersection.
    FieldVector<ctype, dimworld> integrationOuterNormal (const FieldVector<ctype, dim-1>& local) const
    {
      FieldVector<ctype, dimworld> n = _faceInfo[_count].normal;
      n *= geometry().volume();
      return n;
    }

    /*! intersection of codimension 1 of this neighbor with element where iteration started.
       Here returned element is in LOCAL coordinates of the element where iteration started.
     */
    LocalGeometry geometryInInside () const
    {
      return LocalGeometry( _faceInfo[_count].geom_inside );
    }

    /*! intersection of codimension 1 of this neighbor with element where iteration started.
       Here returned element is in LOCAL coordinates of neighbor
     */
    LocalGeometry geometryInOutside () const
    {
      return LocalGeometry( _faceInfo[_count].geom_outside );
    }

    /*! intersection of codimension 1 of this neighbor with element where iteration started.
     */
    Geometry geometry () const
    {
      update();

      std::bitset<dim> shift;
      shift.set();
      shift[_dir] = false;

      Dune::FieldVector<ctype,dimworld> ll(_inside.transformingsubiterator().lowerleft());
      Dune::FieldVector<ctype,dimworld> ur(_inside.transformingsubiterator().upperright());

      if (_face > 0)
      {
        ll = _outside.transformingsubiterator().lowerleft();
        ur = _outside.transformingsubiterator().upperright();
      }
      GeometryImpl _is_global(ll,ur,shift);
      return Geometry( _is_global );
    }

    /** \brief obtain the type of reference element for this intersection */
    GeometryType type () const
    {
      static const GeometryType cube(GeometryType::cube, dim-1);
      return cube;
    }

    //! local index of codim 1 entity in self where intersection is contained in
    int indexInInside () const
    {
      return _count;
    }

    //! local index of codim 1 entity in neighbor where intersection is contained in
    int indexInOutside () const
    {
      // flip the last bit
      return _count^1;
    }

    //! make intersection iterator from entity, initialize to first neighbor
    YaspIntersection (const YaspEntity<0,dim,GridImp>& myself, bool toend) :
      _inside(myself.yaspgrid(), myself.gridlevel(),
              myself.transformingsubiterator()),
      _outside(myself.yaspgrid(), myself.gridlevel(),
               myself.transformingsubiterator()),
      // initialize to first neighbor
      _count(0),
      _dir(0),
      _face(0)
    {
      if (toend)
      {
        // initialize end iterator
        _count = 2*dim;
        return;
      }
      _count = 0;

      // move transforming iterator
      _outside.transformingsubiterator().move(_dir,-1);
    }

    //! copy constructor
    YaspIntersection (const YaspIntersection& it) :
      _inside(it._inside),
      _outside(it._outside),
      _count(it._count),
      _dir(it._dir),
      _face(it._face)
    {}

    //! copy operator
    void assign (const YaspIntersection& it)
    {
      _inside = it._inside;
      _outside = it._outside;
      _count = it._count;
      _dir = it._dir;
      _face = it._face;
    }

  private:
    /* EntityPointers (get automatically updated) */
    mutable YaspEntityPointer<0,GridImp> _inside;  //!< entitypointer to myself
    mutable YaspEntityPointer<0,GridImp> _outside; //!< outside entitypointer
    /* current position */
    uint8_t _count;                                //!< valid neighbor count in 0 .. 2*dim-1
    mutable uint8_t _dir;                          //!< count/2
    mutable uint8_t _face;                         //!< count%2

    /* static data */
    struct faceInfo
    {
      FieldVector<ctype, dimworld> normal;
      LocalGeometryImpl geom_inside;           //!< intersection in own local coordinates
      LocalGeometryImpl geom_outside;          //!< intersection in neighbors local coordinates
    };

    /* static face info */
    static const array<faceInfo, 2*GridImp::dimension> _faceInfo;

    static array<faceInfo, 2*dim> initFaceInfo()
    {
      array<faceInfo, 2*dim> I;
      for (uint8_t i=0; i<dim; i++)
      {
        // compute normals
        I[2*i].normal = 0.0;
        I[2*i+1].normal = 0.0;
        I[2*i].normal[i] = -1.0;
        I[2*i+1].normal[i] = +1.0;

        // determine the shift vector for these intersection
        std::bitset<dim> s;
        s.set();
        s[i] = false;

        // store intersection geometries
        Dune::FieldVector<ctype, dim> ll(0.0);
        Dune::FieldVector<ctype, dim> ur(1.0);
        ur[i] = 0.0;

        I[2*i].geom_inside = LocalGeometryImpl(ll,ur,s);
        I[2*i+1].geom_outside = LocalGeometryImpl(ll,ur,s);

        ll[i] = 1.0;
        ur[i] = 1.0;

        I[2*i].geom_outside = LocalGeometryImpl(ll,ur,s);
        I[2*i+1].geom_inside = LocalGeometryImpl(ll,ur,s);
      }

      return I;
    }
  };

  template<class GridImp>
  const array<typename YaspIntersection<GridImp>::faceInfo, 2*GridImp::dimension>
  YaspIntersection<GridImp>::_faceInfo =
    YaspIntersection<GridImp>::initFaceInfo();

}   // namespace Dune

#endif   // DUNE_GRID_YASPGRIDINTERSECTION_HH
