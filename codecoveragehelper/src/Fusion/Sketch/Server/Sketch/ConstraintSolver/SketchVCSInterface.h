#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSInterface
/// ----------------------------------------------------------------------
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include "SketchSolverGeometry.h"
#include "../Objects/SketchConstraintType.h"
#include <getol.h>

class VCSRigidBody;
class VCSConHandle;
class VCSSystem;

namespace Ns { namespace Comp {
	class VCSInterface;
}}

using namespace Ns::Comp;
using namespace SKs;

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;
	class SketchVCSConstraintData;
	class SketchSolverCircArc;
	class SketchSolverEllipticArc;
	class SketchSolverLine;
	class SketchSolverPoint;
	class SketchVCSStatus;

	/// This is special for sketch constraint solver, it provide some convenient constraints methods 
	/// and delegate details to VCS interfaces.
	class SketchVCSInterface
	{
	public:
		SketchVCSInterface(SketchVCSStatus* pStatus);
		~SketchVCSInterface();

		VCSSystem* vcsSystem();

		/// Invoke VCS to do the solve
		bool solve(unsigned int mode, bool bCacheBeforeSolve);
		/// Minimum movement solving
		bool minMovementSolve(std::vector<VCSConHandle*>& newAddedCons, 
			SketchConstraintType consType,
			bool checkRedundancy = true);

		/// Initial solve for drag begin
		bool solveForDragBegin();

		/// Prioritized drag method which takes a list of bodies
		/// and have some special handles for sketch dragging
		bool prioritizedDrag(const VCSCollection& bodies,  // bodies to drag
			AcGePoint3d& oldPos,			// old position of dragged bodies
			const AcGePoint3d& newPos,	// old position of dragged bodies
			const  AcGeVector3d& viewVec);// view vector

		/// Create fix constraint
		bool fixConstraint(SketchVCSConstraintData& data);
		/// Create on plane constraint
		bool onPlaneConstraint(SketchVCSConstraintData& data);

		/// Create coincident constraint
		bool coincidentConstraint(SketchVCSConstraintData& data);
		bool coincidentConstraintPtPt(SketchVCSConstraintData& data,
			const AcGePoint3d& point1, const AcGePoint3d& point2);
		bool distanceConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable = NULL);

		/// Create angle constraint (could be parallel, perpendicular, etc)
		bool angleConstraint(SketchVCSConstraintData& data);
		/// Create angle constraint (using vector and rigid body directly)
		bool angleConstraintEx(VCSConHandle*& pConHandle, VCSRigidBody* pBody1, VCSRigidBody* pBody2, 
			const AcGeVector3d& vector1, const AcGeVector3d& vector2, double dAngle);

		/// Create parallel constraint
		bool parallelConstraint(SketchVCSConstraintData& data);
		/// Create perpendicular constraint (could be two lines, line and circle, etc)
		bool perpendicularConstraint(SketchVCSConstraintData& data);
		/// Create horizontal constraint (for line)
		bool horizontalConstraint(SketchVCSConstraintData& data);
		/// Create vertical constraint (for line)
		bool verticalConstraint(SketchVCSConstraintData& data);

		/// Create concentric constraint
		bool concentricConstraint(SketchVCSConstraintData& data);
		/// Create collinear constraint (line and Line)
		bool collinearConstraint(SketchVCSConstraintData& data);
		/// Create tangent constraint (line and Line)
		bool tangentConstraint(SketchVCSConstraintData& data);

		/// Create equal constraint (such as line and Line)
		bool equalConstraint(SketchVCSConstraintData& data);
		/// Create midpoint constraint (point on middle of line)
		bool midPointConstraint(SketchVCSConstraintData& data);
		/// Create polygon constraint
		bool polygonConstraint(SketchVCSConstraintData& data);
		/// Create symmetry constraint
		bool symmetryConstraint(SketchVCSConstraintData& data);

		/// Create VCS rigid body
		VCSRigidBody* createRBody();

		/// Create constraint for circle radius
		bool circleRadiusConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable = NULL);
		/// Create constraint for circle diameter
		bool circleDiameterConstraint(SketchVCSConstraintData& data);
		/// Mate point on circle
		bool circleMatePtCir(SketchVCSConstraintData& data,
			AcGePoint3d ptOnCircle,
			VCSRigidBody* pPointOnCircleBody,
			SketchSolverCircArc* pCircArc);
		
		/// Create constraint for ellipse radius
		bool ellipseRadiusConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable = NULL, bool bMajorRadius = true);
		/// Mate point on ellipse
		bool matePtEllipse(SketchVCSConstraintData& data,
			AcGePoint3d ptOnEllipse,
			VCSRigidBody* pPointOnEllipseBody,
			SketchSolverEllipticArc* pEllipArc,
			bool useBias = false,
			AcGePoint3d geBiasPt = AcGePoint3d(),
			bool isFixed = false);

		/// Create variable circle
		bool variableCircle(VCSVarGeomHandle*& varG, VCSRigidBody* body, const AcGeCircArc3d& geCircle);
		/// Create variable ellipse
		bool variableEllipse(VCSVarGeomHandle*& varG, VCSRigidBody* body, const AcGeEllipArc3d& geEllipse);

		/// Create constraint for line's length
		bool lineLengthConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable = NULL);
		/// Create constraint for linear dimension
		bool linearDimensionConstraint(SketchVCSConstraintData& data);
		// Create horizontal/vertical dimension constraint
		bool alignAxisDimensionConstraint(SketchVCSConstraintData& data, const AcGeVector3d& aligAxis);
		/// Check whether there has any invalid constraint
		bool checkInValidConstraint(SketchVCSConstraintData& data);
		/// Create line's mid point body and add related constraint
		VCSRigidBody* createLineMidPointBody(SketchVCSConstraintData& data,
			SketchSolverLine* pLine);
		/// Create construct body for point horizontal constraint
		VCSRigidBody* createConstructBodyHorizontal(SketchVCSConstraintData& data,
			SketchSolverPoint* pPoint);

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSInterface(const SketchVCSInterface&);
		SketchVCSInterface& operator=(const SketchVCSInterface&);

		/// Check geom types, and swap input geom pointers when needed
		bool CheckGeomTypes(SketchSolverGeometry*& pGeomA, 
			SketchSolverGeometry*& pGeomB, 
			ESolverGeometryType geomAType,
			ESolverGeometryType geomBType);

		VCSInterface* m_pVCSInterface;

		/// Below are used for horizontal and vertical constraint
		AcGeVector3d m_horizontalVec;
		AcGeVector3d m_verticalVec;
		VCSRigidBody* m_pHorizontalGeomBody;
		VCSRigidBody* m_pVerticalGeomBody;
		/// Below are for on plane constraint (default plane)
		AcGePlane m_onPlaneDefault;
		VCSRigidBody* m_pXYPlaneBody;
		SketchVCSStatus* m_pStatus;
		AcGeTol m_tol;
	};
}}
