// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
//****************************************************************
//
// --UGGridGeometry
//
//****************************************************************

#include <algorithm>

#include "p2geometry.hh"

///////////////////////////////////////////////////////////////////////
//
// General implementation of UGGridGeometry mydim-> coorddim
//
///////////////////////////////////////////////////////////////////////


template< int mydim, int coorddim, class GridImp>
inline Dune::GeometryType Dune::UGGridGeometry<mydim,coorddim,GridImp>::type() const
{
  switch (mydim)
  {
  case 0 : return GeometryType(0);
  case 1 : return GeometryType(1);
  case 2 :

    switch (UG_NS<coorddim>::Tag(target_)) {
    case UG::D2::TRIANGLE :
      return GeometryType(GeometryType::simplex,2);
    case UG::D2::QUADRILATERAL :
      return GeometryType(GeometryType::cube,2);
    default :
      DUNE_THROW(GridError, "UGGridGeometry::type():  ERROR:  Unknown type "
                 << UG_NS<coorddim>::Tag(target_) << " found!");
    }

  case 3 :
    switch (UG_NS<coorddim>::Tag(target_)) {

    case UG::D3::TETRAHEDRON :
      return GeometryType(GeometryType::simplex,3);
    case UG::D3::PYRAMID :
      return GeometryType(GeometryType::pyramid,3);
    case UG::D3::PRISM :
      return GeometryType(GeometryType::prism,3);
    case UG::D3::HEXAHEDRON :
      return GeometryType(GeometryType::cube,3);
    default :
      DUNE_THROW(GridError, "UGGridGeometry::type():  ERROR:  Unknown type "
                 << UG_NS<coorddim>::Tag(target_) << " found!");

    }
  }

}

template<int mydim, int coorddim, class GridImp>
inline const Dune::FieldVector<typename GridImp::ctype, coorddim>& Dune::UGGridGeometry<mydim,coorddim,GridImp>::
operator [](int i) const
{
  // This geometry is a vertex
  if (mydim==0)
    return reinterpret_cast<FieldVector<typename GridImp::ctype, coorddim>&>(((typename UG_NS<coorddim>::Node*)target_)->myvertex->iv.x);

  // ////////////////////////////////
  //  This geometry is an element
  // ////////////////////////////////
  assert(mydim==coorddim);

  i = UGGridRenumberer<mydim>::verticesDUNEtoUG(i,type());

  if (mode_==element_mode)
    return reinterpret_cast<FieldVector<typename GridImp::ctype, coorddim>&>(UG_NS<coorddim>::Corner(((typename UG_NS<coorddim>::Element*)target_),i)->myvertex->iv.x);

  return coord_[i];
}

template< int mydim, int coorddim, class GridImp>
inline Dune::FieldVector<typename GridImp::ctype, coorddim> Dune::UGGridGeometry<mydim,coorddim,GridImp>::
global(const FieldVector<UGCtype, mydim>& local) const
{
  FieldVector<UGCtype, coorddim> globalCoord;

  if (mode_==element_mode) {

    if (myGrid_->isoparametricOrder_ == 1 || !UG_NS<coorddim>::isBoundaryElement(target_)) {
      UGCtype* cornerCoords[corners()];
      UG_NS<coorddim>::Corner_Coordinates(target_, cornerCoords);

      // Actually do the computation
      UG_NS<coorddim>::Local_To_Global(corners(), cornerCoords, local, globalCoord);
    } else {
      /** \todo Don't recompute the nodes at every call */
      computeIsoparametricNodes();
      return PiecewiseQuadraticGeometry<mydim,coorddim,UGCtype>::global(local, isoparametricNodes_, type());

    }

  } else
  {
    UG_NS<coorddim>::Local_To_Global(corners(), cornerpointers_, local, globalCoord);
  }

  return globalCoord;
}


// Maps a global coordinate within the element to a
// local coordinate in its reference element
template< int mydim, int coorddim, class GridImp>
inline Dune::FieldVector<typename GridImp::ctype, mydim> Dune::UGGridGeometry<mydim,coorddim, GridImp>::
local (const Dune::FieldVector<typename GridImp::ctype, coorddim>& global) const
{
  if (myGrid_->isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry::local() for isoparametric geometry");

  FieldVector<UGCtype, mydim> result;

  if (mode_==element_mode)
  {
    // coorddim*coorddim is an upper bound for the number of vertices
    UGCtype* cornerCoords[coorddim*coorddim];
    UG_NS<coorddim>::Corner_Coordinates(target_, cornerCoords);

    // Actually do the computation
    /** \todo Why is this const_cast necessary? */
    UG_NS<coorddim>::GlobalToLocal(corners(), const_cast<const double**>(cornerCoords), &global[0], &result[0]);
  }
  else
  {
    // Actually do the computation
    /** \todo Why is this const_cast necessary? */
    UG_NS<coorddim>::GlobalToLocal(corners(), const_cast<const double**>(cornerpointers_), &global[0], &result[0]);
  }

  return result;
}

template<int mydim, int coorddim, class GridImp>
inline bool Dune::UGGridGeometry<mydim,coorddim,GridImp>::
checkInside(const Dune::FieldVector<UGCtype, mydim> &loc) const
{
  switch (mydim) {

  case 0 :  // vertex
    return false;
  case 1 :  // line
    return 0 <= loc[0] && loc[0] <= 1;
  case 2 :

    switch (UG_NS<coorddim>::Tag(target_)) {
    case UG::D2::TRIANGLE :
      return 0 <= loc[0] && 0 <= loc[1] && (loc[0]+loc[1])<=1;
    case UG::D2::QUADRILATERAL :
      return 0 <= loc[0] && loc[0] <= 1
             && 0 <= loc[1] && loc[1] <= 1;
    default :
      DUNE_THROW(GridError, "UGGridGeometry::checkInside():  ERROR:  Unknown type "
                 << UG_NS<coorddim>::Tag(target_) << " found!");
    }

  case 3 :
    switch (UG_NS<coorddim>::Tag(target_)) {

    case UG::D3::TETRAHEDRON :
      return 0 <= loc[0] && 0 <= loc[1] && 0 <= loc[2]
             && (loc[0]+loc[1]+loc[2]) <= 1;
    case UG::D3::PYRAMID :
      return 0 <= loc[0] && 0 <= loc[1] && 0 <= loc[2]
             && (loc[0]+loc[2]) <= 1
             && (loc[1]+loc[2]) <= 1;
    case UG::D3::PRISM :
      return 0 <= loc[0] && 0 <= loc[1]
             && (loc[0]+loc[1])<=1
             && 0 <= loc[2] && loc[2] <= 1;
    case UG::D3::HEXAHEDRON :
      return 0 <= loc[0] && loc[0] <= 1
             && 0 <= loc[1] && loc[1] <= 1
             && 0 <= loc[2] && loc[2] <= 1;
    default :
      DUNE_THROW(GridError, "UGGridGeometry::checkInside():  ERROR:  Unknown type "
                 << UG_NS<coorddim>::Tag(target_) << " found!");

    }
  }

}


template< int mydim, int coorddim, class GridImp>
inline typename GridImp::ctype Dune::UGGridGeometry<mydim,coorddim,GridImp>::
integrationElement (const Dune::FieldVector<typename GridImp::ctype, mydim>& local) const
{
  /** \todo No need to recompute the determinant every time on a simplex */
  return std::abs(1/jacobianInverseTransposed(local).determinant());
}


template< int mydim, int coorddim, class GridImp>
inline const Dune::FieldMatrix<typename GridImp::ctype, mydim,mydim>& Dune::UGGridGeometry<mydim,coorddim, GridImp>::
jacobianInverseTransposed (const Dune::FieldVector<typename GridImp::ctype, mydim>& local) const
{
  if (jacobianInverseIsUpToDate_)
    return jac_inverse_;

  if (mode_==element_mode) {

    if (myGrid_->isoparametricOrder_ == 1 || !UG_NS<coorddim>::isBoundaryElement(target_)) {

      // compile array of pointers to corner coordinates
      UGCtype* cornerCoords[corners()];
      UG_NS<coorddim>::Corner_Coordinates(target_, cornerCoords);

      // compute the transformation onto the reference element (or vice versa?)
      UG_NS<coorddim>::Transformation(corners(), cornerCoords, local, jac_inverse_);

    } else {
      /** \todo Don't recompute the nodes at every call */
      computeIsoparametricNodes();

      PiecewiseQuadraticGeometry<mydim,coorddim,UGCtype>::jacobianInverseTransposed(local,
                                                                                    isoparametricNodes_,
                                                                                    type(),
                                                                                    jac_inverse_);
    }

  } else
  {
    // compute the transformation onto the reference element (or vice versa?)
    UG_NS<coorddim>::Transformation(corners(), cornerpointers_, local, jac_inverse_);
  }

  if (type().isSimplex() && myGrid_->isoparametricOrder_ == 1)
    jacobianInverseIsUpToDate_ = true;

  return jac_inverse_;
}


template< int mydim, int coorddim, class GridImp>
void Dune::UGGridGeometry<mydim,coorddim, GridImp>::
computeIsoparametricNodes () const
{
  if (!type().isSimplex())
    DUNE_THROW(NotImplemented, "computeIsoparametricNodes() only for simplices");

  assert(myGrid_);

  // Set UG's currBVP variable to the BVP corresponding to this
  // grid.  This is necessary if we have more than one UGGrid in use.
  UG_NS<coorddim>::Set_Current_BVP(myGrid_->multigrid_->theBVP);

  int numVertices = corners();
  int numEdges    = UG_NS<coorddim>::Edges_Of_Elem(target_);

  // First the nodes on the element corners
  for (int i=0; i<numVertices; i++)
    isoparametricNodes_[i] = operator[](i);

  // Then the nodes on the element edges
  for (int i=0; i<numEdges; i++) {

    int ugEdgeNumber = UGGridRenumberer<coorddim>::edgesDUNEtoUG(i, type());

    const typename UG_NS<coorddim>::Vertex* v0 = UG_NS<coorddim>::Corner(target_,UG_NS<coorddim>::Corner_Of_Edge(target_, ugEdgeNumber,0))->myvertex;
    const typename UG_NS<coorddim>::Vertex* v1 = UG_NS<coorddim>::Corner(target_,UG_NS<coorddim>::Corner_Of_Edge(target_, ugEdgeNumber,1))->myvertex;

    if (UG_NS<coorddim>::Edge_On_Bnd(target_,ugEdgeNumber)) {

      if (coorddim==2) {         // triangle

        UG::D2::BNDP* bndp = UG::D2::BNDP_CreateBndP(myGrid_->multigrid_->theHeap,v0->bv.bndp, v1->bv.bndp,0.5);
        if (bndp == NULL)
          DUNE_THROW(GridError, "UG::D" << coorddim << "::BNDP_CreateBndP() returned NULL!");

        if (UG::D2::BNDP_Global(bndp,&isoparametricNodes_[i+numVertices][0]))
          DUNE_THROW(GridError, "UG::D" << coorddim << "::BNDP_Global() returned nonzero error code!");

        /** \bug The following line has to be there to avoid a memory leak,
            but it makes the code crash! */
        //UG::D2::BNDP_Dispose(myGrid_->multigrid_->theHeap, bndp);

      } else {        // tetrahedron

        UG::D3::BNDP* bndp = UG::D3::BNDP_CreateBndP(myGrid_->multigrid_->theHeap,v0->bv.bndp, v1->bv.bndp,0.5);
        if (bndp == NULL)
          DUNE_THROW(GridError, "UG::D3::BNDP_CreateBndP() returned NULL!");

        if (UG::D3::BNDP_Global(bndp,&isoparametricNodes_[i+numVertices][0]))
          DUNE_THROW(GridError, "UG::D3::BNDP_Global() returned nonzero error code!");

        /** \bug The following line has to be there to avoid a memory leak,
            but it makes the code crash! */
        //UG::D3::BNDP_Dispose(myGrid_->multigrid_->theHeap, bndp);
      }

    } else {

      // Not a boundary egde: extra node is the edge midpoint
      for (int j=0; j<coorddim; j++)
        isoparametricNodes_[i+numVertices][j] = (v0->iv.x[j] + v1->iv.x[j]) / 2;

    }

  }

}

///////////////////////////////////////////////////////////////////////
//
// The specializations 1->2, 2->3
// (only methods that are not yet defined in header file)
//
///////////////////////////////////////////////////////////////////////


// Specialization for dim==1, coorddim==2.  This is necessary
// because we specialized the whole class
template <class GridImp>
inline Dune::FieldVector<typename GridImp::ctype, 2> Dune::UGGridGeometry<1,2,GridImp>::
global(const FieldVector<typename GridImp::ctype, 1>& local) const
{
  FieldVector<UGCtype, 2> globalCoord;

  if (isoparametricOrder_ == 1) {

    globalCoord[0] = local[0]*coord_[1][0] + (1-local[0])*coord_[0][0];
    globalCoord[1] = local[0]*coord_[1][1] + (1-local[0])*coord_[0][1];

  } else
    return PiecewiseQuadraticGeometry<1,2,typename GridImp::ctype>::global(local, coord_,type());

  return globalCoord;
}

// Specialization for dim==2, coorddim==3.  This is necessary
// because we specialized the whole class
template <class GridImp>
inline Dune::FieldVector<typename GridImp::ctype, 3> Dune::UGGridGeometry<2,3,GridImp>::
global(const FieldVector<typename GridImp::ctype, 2>& local) const
{

  FieldVector<UGCtype, 3> result;

  if (isoparametricOrder_ == 1) {

    if (elementType_.isSimplex()) {

      for (int i=0; i<3; i++)
        result[i] = (1.0-local[0]-local[1])*coord_[0][i]
                    + local[0]*coord_[1][i]
                    + local[1]*coord_[2][i];

    } else {

      // quadrilateral
      for (int i=0; i<3; i++)
        result[i] = (1.0-local[0])*(1.0-local[1])*coord_[0][i]
                    + local[0]*(1.0-local[1])*coord_[1][i]
                    + local[0]*local[1]*coord_[2][i]
                    + (1.0-local[0])*local[1]*coord_[3][i];

    }

  } else
    return PiecewiseQuadraticGeometry<2,3,typename GridImp::ctype>::global(local, coord_,type());

  return result;

}


template <class GridImp>
inline Dune::FieldVector<typename GridImp::ctype, 2> Dune::UGGridGeometry<2,3,GridImp>::
local(const FieldVector<typename GridImp::ctype, 3>& global) const
{
  if (isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry<2,3>::local() for isoparametric geometry");

  // The UG method UG_GlobalToLocalBnd pretends to do what this method does,
  // but in reality it is buggy!
  FieldVector<UGCtype,2> result;

  FieldMatrix<UGCtype,2,2> M;
  FieldVector<UGCtype,2> partialDiff;

  FieldVector<UGCtype,3> diff = global;
  diff -= coord_[0];

  if (elementType_.isTriangle()) {

    /* the simplex case */
    FieldMatrix<UGCtype,3,2> matrix;
    for (int i=0; i<3; i++) {
      matrix[i][0] = coord_[1][i] - coord_[0][i];
      matrix[i][1] = coord_[2][i] - coord_[0][i];
    }

    // Simply regard the projection onto the x-y plane
    M[0] = matrix[0];
    M[1] = matrix[1];
    partialDiff[0] = diff[0];
    partialDiff[1] = diff[1];

    if (M.determinant()!=0) {
      M.solve(result, partialDiff);
      return result;
    }

    // The triangle is orthogonal to the x-y plane.  try the x-z plane
    M[1]           = matrix[2];
    partialDiff[1] = diff[2];

    if (M.determinant()!=0) {
      M.solve(result, partialDiff);
      return result;
    }

    // Last attempt: the y-z plane
    M[0]           = matrix[1];
    partialDiff[0] = diff[1];

    if (M.determinant()!=0) {
      M.solve(result, partialDiff);
      return result;
    }

    // There must be something wrong here
    assert(false);
  } else {
    assert(elementType_.isQuadrilateral());

    //  Newton - Iteration zum Invertieren der Abbildung f.
    double err;
    FieldVector<UGCtype,3> map(0);
    int count = 0 ;

    FieldMatrix<double,3,3> Df;                // set in method map2worldlinear
    Dune::FieldVector<double,3> normal_;       // in method map2worldnormal
    Dune::FieldMatrix<double,4,3> _b;
    Dune::FieldMatrix<double,3,3> _n;

    // buildmapping
    for (int i=0; i<3; i++) {

      _b [0][i] = coord_[0] [i] ;
      _b [1][i] = coord_[1] [i] - coord_[0] [i] ;
      _b [2][i] = coord_[3] [i] - coord_[0] [i] ;
      _b [3][i] = coord_[2] [i] - coord_[3] [i] - _b [1][i] ;

    }

    _n [0][0] = _b [1][1] * _b [2][2] - _b [1][2] * _b [2][1] ;
    _n [0][1] = _b [1][2] * _b [2][0] - _b [1][0] * _b [2][2] ;
    _n [0][2] = _b [1][0] * _b [2][1] - _b [1][1] * _b [2][0] ;
    _n [1][0] = _b [1][1] * _b [3][2] - _b [1][2] * _b [3][1] ;
    _n [1][1] = _b [1][2] * _b [3][0] - _b [1][0] * _b [3][2] ;
    _n [1][2] = _b [1][0] * _b [3][1] - _b [1][1] * _b [3][0] ;
    _n [2][0] = _b [3][1] * _b [2][2] - _b [3][2] * _b [2][1] ;
    _n [2][1] = _b [3][2] * _b [2][0] - _b [3][0] * _b [2][2] ;
    _n [2][2] = _b [3][0] * _b [2][1] - _b [3][1] * _b [2][0] ;

    do {
      FieldVector<UGCtype,3> upd ;
      //map2worldnormal (map_[0],map_[1],map_[2], upd, normal_,_b, _n) ;
      for (int i=0; i<3; i++)
        normal_ [i] = -(_n [0][i] + _n [1][i] * map[0] + _n [2][i] * map[1]);

      for (int i=0; i<3; i++)
        upd[i] = _b [0][i] + map[0] * _b [1][i] + map[1] * _b [2][i] + map[0]*map[1] * _b [3][i] + map[2]*normal_[0];

      //inverse (map_, normal_, _b, _n, Df,Dfi) ;
      for (int i=0; i<3; i++) {
        Df[i][0] = _b [1][i] + map[1] * _b [3][i]+ map[2]*_n[1][i] ;
        Df[i][1] = _b [2][i] + map[0] * _b [3][i]+ map[2]*_n[2][i] ;
        Df[i][2] = normal_[i];
      }

      upd -= global;

      FieldVector<UGCtype,3> c(0);
      Df.solve(c,upd);

      map -= c;

      err = c.two_norm();

      if (count++ > 100)
        DUNE_THROW(NotImplemented, "Newton solver did not converge!");

    } while (err > 1e-5) ;

    result[0]=map[0];
    result[1]=map[1];

  }

  return result;
}

template <class GridImp>
inline typename GridImp::ctype Dune::UGGridGeometry<1,2,GridImp>::
integrationElement (const Dune::FieldVector<typename GridImp::ctype, 1>& local) const
{
  if (isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry<1,2>::integrationElement() for isoparametric geometry");

  // We could call UG_NS<2>::SurfaceElement, but this is faster, and not more complicated
  FieldVector<UGCtype, 2> diff = coord_[0];
  diff -= coord_[1];
  return diff.two_norm();
}

template <class GridImp>
const Dune::FieldMatrix<typename GridImp::ctype,1,1>& Dune::UGGridGeometry<1,2,GridImp>::
jacobianInverseTransposed(const Dune::FieldVector<typename GridImp::ctype, 1>& local) const
{
  if (isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry<1,2>::jacobianInverseTransposed() for isoparametric geometry");

  jacobianInverseTransposed_[0][0] = 1 / (coord_[0]-coord_[1]).two_norm();
  return jacobianInverseTransposed_;
}

template <class GridImp>
inline typename GridImp::ctype Dune::UGGridGeometry<2,3,GridImp>::
integrationElement (const Dune::FieldVector<typename GridImp::ctype, 2>& local) const
{
  if (isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry<2,3>::integrationElement() for isoparametric geometry");

  // The cast in the second argument works because a std::array<FieldVector<T,3>,4>
  // has the same memory layout as a T[4][3].
  return UG_NS<3>::SurfaceElement(corners(), (const double(*)[3])&coord_,&local[0]);
}

template <class GridImp>
const Dune::FieldMatrix<typename GridImp::ctype,2,2>& Dune::UGGridGeometry<2,3,GridImp>::
jacobianInverseTransposed(const Dune::FieldVector<typename GridImp::ctype, 2>& local) const
{
  if (isoparametricOrder_ != 1)
    DUNE_THROW(NotImplemented, "UGGridGeometry<2,3>::jacobianInverseTransposed() for isoparametric geometry");

  // I don't really know how to implement this for quadrilateral faces,
  // especially since they may be nonplanar.
  if (!type().isTriangle())
    DUNE_THROW(NotImplemented, "jacobianInverse only implemented for triangular faces!");

  // The spatial triangle is first mapped isometrically onto the plane.  We map
  // the first vertex onto the origin, the second one on the positive x-axis,
  // and the third one such that is has positive y-coordinate.  Then we call
  // the UG-routine for planar triangles.  This is certainly not the most elegant
  // way, but the first one that comes to my mind.

  double l0 = (coord_[2]-coord_[1]).two_norm();
  double l1 = (coord_[2]-coord_[0]).two_norm();
  double l2 = (coord_[1]-coord_[0]).two_norm();

  double q0 = (l2*l2 - l0*l0 + l1*l1) / (2*l2);
  double h  = sqrt(l1*l1 - q0*q0);

  FieldVector<double,2> p0(0);
  FieldVector<double,2> p1(0);    p1[0] = l2;
  FieldVector<double,2> p2(0);    p2[0] = q0;   p2[1] = h;

  // Check that this was really an isometry
  assert( std::abs(p2.two_norm()      - l1) < 1e-6 );
  assert( std::abs((p2-p1).two_norm() - l0) < 1e-6 );

  double* cornerCoords[3] = {&p0[0], &p1[0], &p2[0]};

  UG_NS<2>::Transformation(3, cornerCoords, local, jacobianInverseTransposed_);
  return jacobianInverseTransposed_;
}
