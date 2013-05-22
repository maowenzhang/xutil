#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverGeometry (point, line, circle, etc)
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
	class Line3D;
}}

using namespace Ns;
using namespace Ns::Geometry;


namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;
	class SketchVCSConstraintData;
	class SketchVCSDraggingData;
	class SketchSolverReactor;
	class SketchSolverPoint;
	
	/// Solver geometry for sketch line
	class SketchSolverLine : public SketchSolverGeometry
	{
	public:
		SketchSolverLine(PassiveRef<Line3D> geometry, SketchConstraintSolverContext* pContext);
		virtual ~SketchSolverLine();

		/// Update value based on transformation from solving result
		virtual bool updateValue(const AcGeMatrix3d& mat);
		/// Check before update value
		virtual bool checkPreUpdateValue();
		/// Handles for dragging
		virtual bool addHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Add extra geometries for dragging
		virtual bool addExtraGeomsForDragging(SketchVCSDraggingData& dragData);
		/// Add extra handles for dragging (when drag itself or others)
		virtual bool addExtraHandlesForDragging(SketchVCSDraggingData& dragData);
		virtual void addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c);
		/// Notification when related item is dragged
		virtual bool addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem);

		virtual const AcGePoint3d& position();
		virtual const AcGeLine3d& line();
		virtual AcGeVector3d lineDirection();
		const AcGeLineSeg3d& lineSeg();
		
		double length() const;

		SketchSolverGeometry* solverGeomStartPt();
		SketchSolverGeometry* solverGeomEndPt();

		virtual PassiveRef<Geometry3D> geometry();

		virtual bool degenerate();

		/// On notify from reactor
		virtual int onReactorNotify(int iVCSMessage);

		/// Add VCS reactor when needed
		virtual void addReactor();
		/// Add constraint to fix line's length
		bool fixLength();
		/// Get midpoint body (only for internal use)
		VCSRigidBody* getMidPointBody();

		virtual bool useCombinedBody() const;
		virtual void useCombinedBody(bool bVal);

	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data);

	private:
		void createReactor();
		/// Fix another end point when drag arc
		void fixAnotherEndPointWhenDrag(SketchSolverGeometry* pSolverPt);

		/// Associated geometry
		PassiveRef<Line3D> m_geometry;
		AcGeLine3d m_tempLine;
		// VCS reactor
		SketchSolverReactor* m_pReactor;
		AcGePoint3d m_position;
		VCSRigidBody* m_pMidPointBody;
		bool m_bUseCombinedBody;
	};
}}
