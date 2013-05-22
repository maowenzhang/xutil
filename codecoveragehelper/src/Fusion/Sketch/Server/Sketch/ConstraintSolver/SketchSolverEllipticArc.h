#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverEllipticArc (ellipse arc)
/// ----------------------------------------------------------------------

#include <Server/DataModel/Refs/PassiveRef.h>
#include "SketchSolverGeometry.h"

class AcGeMatrix3d;
class AcGeEllipArc3d;
class VCSRigidBody;
class VCSVarGeomHandle;

namespace Ns { namespace Geometry {
	class Geometry3D;
	class EllipticArc3D;
}}

using namespace Ns;
using namespace Ns::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;
	class SketchVCSConstraintData;
	class SketchVCSDraggingData;

	/// Solver geometry for sketch EllipticArc
	class SketchSolverEllipticArc : public SketchSolverGeometry
	{
	public:
		SketchSolverEllipticArc(PassiveRef<EllipticArc3D> geometry, SketchConstraintSolverContext* pContext);
		virtual ~SketchSolverEllipticArc();

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
		/// Add extra geometries for dragging
		virtual bool addExtraGeomsForDragging(SketchVCSDraggingData& dragData);
		/// Notification when related item is dragged
		virtual bool addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem);

		/// Get AcGe object
		virtual const AcGeEllipArc3d& ellipArc() const;

		/// Solver geometry for center point
		virtual SketchSolverGeometry* centerPointSolverGeometry();

		/// Get variable ellipse
		VCSVarGeomHandle* variableEllipseHandle();

		VCSRigidBody* internalBodyOnEllip(SketchVCSConstraintData& data, AcGePoint3d& ptOnEllipse);
		VCSRigidBody* internalBodyOnEllipMajor(SketchVCSConstraintData& data, double posParam = -1.0);
		VCSRigidBody* internalBodyOnEllipMinor(SketchVCSConstraintData& data, double posParam = -1.0);
		AcGePoint3d getEllipMajorPt();
		AcGePoint3d getEllipMinorPt();

		/// Set invariable ellipse to make ellipse radius doesn't change in some cases
		void setInvariable();

		/// Add entire geometry to drag
		void addEntireToDrag(SketchVCSDraggingData& data);

		virtual bool degenerate();

		/// Get variable ellipse
		virtual VCSVarGeomHandle* varGeomHandle();
		/// Set VCS body as grounded
		virtual void setGrounded(bool bGrounded);

		virtual PassiveRef<Geometry3D> geometry();
		SketchSolverGeometry* endPoint();
		SketchSolverGeometry* startPoint();

	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data);

		/// Update cached value
		virtual void updateCachedValue();

	private:
		bool isDragChangingMajorAxis(SketchVCSDraggingData& dragData, double& posParam);

		/// Associated geometry
		PassiveRef<EllipticArc3D> m_geometry;
		/// Handle for variable ellipse
		VCSVarGeomHandle* m_pVcsVarEllipse;
		/// VCS rigid body for point on ellipse
		VCSRigidBody* m_pInternalBodyOnEllipMajor;
		VCSRigidBody* m_pInternalBodyOnEllipMinor;

		AcGeEllipArc3d m_ellipticArc;
	};
}}
