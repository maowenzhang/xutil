#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverGeometry (point, line, circle, etc)
/// ----------------------------------------------------------------------

#include <gepnt3d.h>
#include <geline3d.h>
#include <gearc3d.h>
#include <geell3d.h>
#include <genurb3d.h>
#include <vector>
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
	class Spline3D;
    class Curve3D;
}}

using namespace Ns;
using namespace Ns::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;
	class SketchVCSConstraintData;
	class SketchVCSDraggingData;
	class SketchSolverReactor;

	/// Solver geometry for sketch circle arc
	class SketchSolverSpline : public SketchSolverGeometry
	{
	public:
		SketchSolverSpline(PassiveRef<Spline3D> geometry, SketchConstraintSolverContext* pContext);
		virtual ~SketchSolverSpline();

		/// Update value based on transformation from solving result
		virtual bool updateValue(const AcGeMatrix3d& mat);
		/// Handles for dragging
		virtual bool addHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Add extra geometries for dragging
		virtual bool addExtraGeomsForDragging(SketchVCSDraggingData& dragData);
		/// Add extra handles for dragging (when drag itself or others)
		virtual bool addExtraHandlesForDragging(SketchVCSDraggingData& dragData);
		virtual void addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c);
		/// Notification when related item is dragged
		virtual bool addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem);
		virtual PassiveRef<Geometry3D> geometry();

		virtual bool degenerate();

		/// On notify from reactor
		virtual int onReactorNotify(int iVCSMessage);

		/// Get extension curve
		virtual SketchVCSExtCurve* extCurve();

		/// On rigid tester
		virtual bool isExternalRigid();

		/// Get ext Geometry
		virtual VCSExtGeometry* extGeometry();

        bool needReverse() const;
        void needReverse(bool reverse);
        AcGeNurbCurve3d computeSpline(std::vector<AcGePoint3d>& fitPts, bool geometryPosition = false);

		void hasVCSConstraint(bool val);

	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data);
		
	private:
        AcGeCircArc3d updatedCircArc(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const;
        AcGeEllipArc3d updatedEllipArc(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const;
        AcGeLineSeg3d updatedLineSeg(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const;
		void createReactor();
		void getAllFitPoints();
        void getAllTangents();
        void getAllCurvatures();
		bool generateNewCurve(AcGeNurbCurve3d& newCurve);
		void setposition();

		/// Associated geometry
		PassiveRef<Spline3D> m_geometry;
		// VCS reactor
		SketchSolverReactor* m_pReactor;
		// Ext curve
		std::vector<SketchVCSExtCurve*> m_extCurves;

		std::vector<SketchSolverReactor*> m_reactors;
		bool m_bComputed;
		std::vector<SketchSolverGeometry*> m_FitPoints;
		std::vector<AcGePoint3d> m_FitPointsValue;

		std::vector<SketchSolverGeometry*> m_tangents;
		std::map<int, AcGeVector3d> m_tangentsValue;

		std::vector<SketchSolverGeometry*> m_curvatures;
		std::map<int, AcGeVector3d> m_curvaturesValue;

        bool m_needReverse;

		// Check whether has constraint on spline (coincident, tangent, etc.)
		// Add this flag in order to decide whether formRigidSet in reactor:  
		// 1. if formRigidSet, drag fails when has 2 fit points symmetry (issue in VCS)
		// 2. if without formRigidSet, drag fails when has a line's end point coincident on spline
		bool m_bHasVCSConstraint;
	};
}}
