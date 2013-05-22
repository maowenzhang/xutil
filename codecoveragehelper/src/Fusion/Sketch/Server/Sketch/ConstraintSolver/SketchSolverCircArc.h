#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverEllipticArc (ellipse arc)
/// ----------------------------------------------------------------------

#include <gepnt3d.h>
#include <geline3d.h>
#include <gearc3d.h>
#include <geell3d.h>
#include <Server/DataModel/Refs/PassiveRef.h>
#include "SketchSolverGeometry.h"

class AcGeMatrix3d;
class VCSRigidBody;
class VCSVariable;
class VCSVarGeomHandle;
class VCSCollection;

namespace Ns { namespace Geometry {
	class Geometry3D;
	class Point3D;
	class CircArc3D;
	class EllipticArc3D;
}}
namespace SKs
{
    class CurveContinuityConstraint;
}

using namespace Ns;
using namespace Ns::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;
	class SketchVCSConstraintData;
	class SketchVCSDraggingData;
	class SketchSolverPoint;

	/// Solver geometry for sketch circle arc
	class SketchSolverCircArc : public SketchSolverGeometry
	{
	public:
		SketchSolverCircArc(PassiveRef<CircArc3D> geometry, SketchConstraintSolverContext* pContext);
		virtual ~SketchSolverCircArc();

		/// Update value based on transformation from solving result
		virtual bool updateValue(const AcGeMatrix3d& mat);
		/// Update value to last solved
		virtual bool updateValueToLastSolved();
		/// Check before update value
		virtual bool checkPreUpdateValue();
		/// Handles for dragging
		virtual bool addHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Add extra handles for dragging (when drag itself or others)
		virtual bool addExtraHandlesForDragging(SketchVCSDraggingData& dragData);
		virtual void addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c);
		/// Notification when related item is dragged
		virtual bool addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem);

		/// Solver geometry for center point
		virtual SketchSolverGeometry* centerPointSolverGeometry();
		/// Get AcGe object
		virtual const AcGeCircArc3d& circArc() const;

		/// Get variable circle
		VCSVarGeomHandle* variableCircleHandle();

		VCSRigidBody* vcsBodyPtOnCircle(SketchVCSConstraintData& data);
		bool isInternalPtOnCircle(VCSRigidBody* pBody);

		AcGePoint3d ptOnCircle();

		/// Set invariable circle to make circle radius doesn't change in some cases
		void setInvariable(bool bNotSetIfHasVariableRadiusCons = true);

		/// Add entire geometry to drag
		void addEntireToDrag(SketchVCSDraggingData& data);

		virtual bool degenerate();

		/// Indicate whether set variable radius constraint
		void hasVariableRadiusCons(bool val);
		bool hasVariableRadiusCons() const;

		/// Indicate whether has constraint on circle/arc
		void hasConsOnCircumference(bool val);
		bool hasConsOnCircumference();

		/// Get variable circle
		virtual VCSVarGeomHandle* varGeomHandle();
		/// Set VCS body as grounded
		virtual void setGrounded(bool bGrounded);
		
		virtual bool isGrounded();

		SketchSolverGeometry* endPoint();
		SketchSolverGeometry* startPoint();

		virtual PassiveRef<Geometry3D> geometry();
	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data);

		/// Update cached value
		virtual void updateCachedValue();

	private:
		/// Associated geometry
		PassiveRef<CircArc3D> m_geometry;
		/// Handle for variable circle
		VCSVarGeomHandle* m_pVcsVarCircle;
		/// VCS rigid body for point on circle
		VCSRigidBody* m_pVcsRigidBodyPtOnCircle;
		/// Indicate whether set variable radius constraint (such as: equal constraint on two circles)
		bool m_bHasVariableRadiusCons;
		/// Indicate whether has constraint on circle/arc
		bool m_bHasConsOnCircumference;
		AcGeCircArc3d m_circArc;
	};	
}}
