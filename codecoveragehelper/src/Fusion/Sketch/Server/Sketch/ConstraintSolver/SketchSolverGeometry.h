#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverGeometry (point, line, circle, etc)
/// ----------------------------------------------------------------------

// Define _MAC for including VCS headers
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <gepnt3d.h>
#include <geline3d.h>
#include <gearc3d.h>
#include <geell3d.h>
#include <Server/DataModel/Refs/PassiveRef.h>
#include <VCSMath.h>
#include <vector>

class AcGeMatrix3d;
class VCSRigidBody;
class VCSVariable;
class VCSVarGeomHandle;
class VCSCollection;
class VCSExtGeometry;

namespace Ns { namespace Geometry {
	class Geometry3D;
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
	class SketchSolverReactor;
	class SketchVCSExtCurve;
	class SketchVCSExtRigidTester;
	class SketchSolverGeometry;

	typedef std::vector<SketchSolverGeometry*> SketchSolverGeoms;

	enum ESolverGeometryType
	{
		eSolverGeomNone,
		eSolverGeomPoint,
		eSolverGeomLine,
		eSolverGeomCirArc,
		eSolverGeomEllipticArc,
		eSolverGeomSpline
	};

	/// This class is used to represent sketch geometry (line, point, sp-line, etc) that join the VCS solving. 
	/// It contains data member “VCSRigidBody”, and may also contains VCSReactor. (Such as sp-line, ellipse)
	/// This class is used to store sketch geometry data, set data to VCS solving and get updated data back 
	/// to sketch geometry.
	/// We have sub-classes “SketchSolverPoint, SketchSolverLine, SketchSolverSpline, etc” as different types have different details.
	/// (Some need reactor, some need add extra constraints, etc)
	class SketchSolverGeometry
	{
	public:
		SketchSolverGeometry(SketchConstraintSolverContext* pContext, ESolverGeometryType geomType);
		virtual ~SketchSolverGeometry();

		/// Update value based on transformation from rigid body
		bool updateValueBySolvedBody(bool bUpdateToLastSolved = false);

		/// Update value
		virtual bool updateValue(const AcGeMatrix3d& mat) = 0;
		/// Update value to last solved, when fail in dragging, we need to update value 
		/// to last solved as transaction aborted last modification to geometry.
		virtual bool updateValueToLastSolved();

		/// Check before update value
		virtual bool checkPreUpdateValue();

		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool addAdditionalConstraints(SketchVCSConstraintData& data);

		/// Handles for dragging (when drag itself)
		virtual bool addHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Add extra geometries for dragging (when drag itself)
		virtual bool addExtraGeomsForDragging(SketchVCSDraggingData& dragData);
		/// Add extra handles for dragging (when drag itself or others)
		virtual bool addExtraHandlesForDragging(SketchVCSDraggingData& dragData);
		/// Notification when related item is dragged
		virtual bool addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem);

		virtual void addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c);

		/// Solver geometry type
		ESolverGeometryType geomType() const;

		/// Get VCS rigid body
		virtual VCSRigidBody* vcsBody();
		
		/// Get temporary geometries for creating constraints, sub classes will override them as needed
		virtual const AcGePoint3d& position();
		void position(const AcGePoint3d& pos);

		virtual const AcGeLine3d& line();
		virtual AcGeVector3d lineDirection();
		virtual const AcGeCircArc3d& circArc() const;
		virtual const AcGeEllipArc3d& ellipArc() const;
		/// Get solver geometry of center point (current especially for ellipse, circle)
		virtual SketchSolverGeometry* centerPointSolverGeometry();
		VCSMPoint3d positionEx();
		VCSMLine3d lineEx();

		/// Get concrete solver geometry
		template <class T>
		T* solverGeom(ESolverGeometryType eType)
		{
			if (eType == this->geomType())
			{
				return static_cast<T*>(this);
			}
			NEUTRON_ASSERT(false);
			return NULL;
		}

		/// Set VCS body as grounded
		virtual void setGrounded(bool bGrounded);
		/// Check whether grounded
		virtual bool isGrounded();

		/// Dump info
		virtual Ns::IString dumpInfo();

		virtual bool addOnPlaneConstraint(SketchVCSConstraintData& data);

		virtual bool degenerate() = 0;
		virtual PassiveRef<Geometry3D> geometry() = 0;

		/// On notify from reactor
		virtual int onReactorNotify(int iVCSMessage);

		/// Get extension curve
		virtual SketchVCSExtCurve* extCurve();

		/// On rigid tester
		virtual bool isExternalRigid();

		/// Get ext Geometry
		virtual VCSExtGeometry* extGeometry();

		/// Get variable geometry (circle, ellipse vcs geometry handle)
		virtual VCSVarGeomHandle* varGeomHandle();

		/// Get updated position based on vcs body
		AcGePoint3d updatedPosition();
		/// Add VCS reactor when needed
		virtual void addReactor();

		/// Indicate whether share same body for multi geometries 
		/// (current used by line, point when line has dimension)
		virtual bool useCombinedBody() const;
		virtual void useCombinedBody(bool bVal);
		virtual void useCombinedBody(SketchSolverGeometry* pGeom);

		/// Update cached value
		virtual void updateCachedValue();

	protected:
		/// Add additional constraints, such as constraint two end points for sketch line
		virtual bool onAddAdditionalConstraints(SketchVCSConstraintData& data) = 0;

		/// Solver context
		SketchConstraintSolverContext* m_pContext;
		AcGePoint3d m_position;

	private:
		/// Current we don't need copy and assignment constructors
		SketchSolverGeometry(const SketchSolverGeometry&);
		SketchSolverGeometry& operator=(const SketchSolverGeometry&);

		/// Initial
		void initialize();

		ESolverGeometryType m_geomType;
		/// Related VCS rigid body
		VCSRigidBody* m_pVcsRigidBody;

		/// Default geometries, used to unify the interfaces
		static AcGeLine3d m_defaultLine;
		static AcGeCircArc3d m_defaultArc;
		static AcGeEllipArc3d m_defaultEllipArc;

		/// Indicate whether has added
		bool m_bAddedAdditionalConstraints;

		/// Indicate whether has been updated before, used to update cached value when needed
		bool m_bUpdatedBefore;
	};
}}
