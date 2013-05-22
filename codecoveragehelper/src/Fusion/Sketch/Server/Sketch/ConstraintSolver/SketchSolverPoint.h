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
}}

using namespace Ns;
using namespace Ns::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;
	class SketchVCSConstraintData;
	class SketchVCSDraggingData;
	class SketchSolverReactor;
	class SketchSolverLine;

	/// Solver geometry for sketch point
	class SketchSolverPoint : public SketchSolverGeometry
	{
	public:
		SketchSolverPoint(PassiveRef<Point3D> geometry, SketchConstraintSolverContext* pContext);
		virtual ~SketchSolverPoint();

		/// Update value based on transformation from solving result
		virtual bool updateValue(const AcGeMatrix3d& mat);
		/// Update value to last solved
		virtual bool updateValueToLastSolved();

		/// Handles for dragging
		virtual bool addHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Add extra geometries for dragging
		virtual bool addExtraGeomsForDragging(SketchVCSDraggingData& dragData);

		/// Get VCS rigid body
		virtual VCSRigidBody* vcsBody();

		virtual const AcGePoint3d& position();
		virtual PassiveRef<Geometry3D> geometry();
		
		virtual bool addOnPlaneConstraint(SketchVCSConstraintData& data);

		virtual bool degenerate();

		VCSRigidBody* getConstructBodyHorizontal();
		VCSRigidBody* getConstructBodyVertical();

		virtual bool useCombinedBody() const;
		virtual void useCombinedBody(SketchSolverGeometry* pGeom);

	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data);

		/// Update cached value
		virtual void updateCachedValue();

	private:
		/// Associated geometry
		PassiveRef<Point3D> m_geometry;
		VCSRigidBody* m_pConsBodyHorizontal;
		VCSRigidBody* m_pConsBodyVertical;
		SketchSolverGeometry* m_pCombinedLine;
	};	
}}
