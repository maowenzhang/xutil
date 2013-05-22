#pragma once

#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

/// ----------------------------------------------------------------------
/// This file is used to keep SketchConstraintSolverContext
/// ----------------------------------------------------------------------
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"

#include <Server/DataModel/Refs/PassiveRef.h>
#include <Fusion/Sketch/Server/Sketch/Objects/MSketch.h>
#include <map>
#include <vector>
#include <getol.h>

namespace SKs {
	class ISketchConstraint;
    class CurveContinuityConstraint;
}

namespace SKs { namespace Geometry {
	class MSketch;
}}

namespace Ns { namespace Geometry {
	class Geometry3D;
	class Point3D;
}}

using namespace Ns;
using namespace Ns::Geometry;
using namespace SKs::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;
	class SketchVCSInterface;
	class SketchVCSStatus;

	/// This class is used to keep some data that needed during constraint solving, 
	/// and also contains some implementation details for its parent class SketchConstraintSolver.
	class SketchConstraintSolverContext
	{
	public:
		SketchConstraintSolverContext();
		~SketchConstraintSolverContext();

		///
		bool solve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons);

		/// Load data from sketch
		bool initialize(PassiveRef<MSketch>& rSketch, 
			const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag,
			ISketchConstraint* pNewAddCons = NULL);

		/// Get solver geometry based on input sketch geometry, create one if hasn't
		SketchSolverGeometry* solverGeometry(const PassiveRef<Geometry3D>& rGeom);

		/// Get VCSInterface
		SketchVCSInterface* vcsInterface();

		/// Update value based on solver result
		/// bSkipUpdatedCurveInSolve: skip updated curve (e.g. spline) which is computed during solving
		/// In order to reduce solving times, skip updating it.
		bool updateSolverGeometries(bool bSolveSuccess, bool bSkipUpdatedCurveInSolve = true);

		/// Add updated sketch point
		void addUpdatedPoint(PassiveRef<Point3D> pt);
		/// Dragging, on mouse move
		bool drag(AcGePoint3d& oldPos, const AcGePoint3d& newPos, const AcGeVector3d& viewVec);
		/// On Drag begin
		bool dragBegin(PassiveRef<MSketch>& rSketch,
			const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag, 
			const AcGePoint3d& startPt);

		///Get the geometries changed during drag
		void getChangedGeometries(std::vector< PassiveRef<Geometry3D> >& gemosChanged);

		/// Get constraint data
		SketchVCSConstraintData& consData();

		/// Get dragging data
		SketchVCSDraggingData& dragData();

		/// Check whether current is dragging
		bool isDragging() const;

		/// Get status
		SketchVCSStatus* status();

		/// Check whether the geometries are not expected(degenerated) after solving
		bool isRelationMet();

		double tol() const;
		const AcGeTol& tolEx() const;

        PassiveRef<MSketch> sketch() const;
	private:
		/// Current we don't need copy and assignment constructors
		SketchConstraintSolverContext(const SketchConstraintSolverContext&);
		SketchConstraintSolverContext& operator=(const SketchConstraintSolverContext&);

		bool needVCSSolve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons);
		/// Get solver geometry based on input sketch geometry, create one if hasn't
		SketchSolverGeometry* createSolverGeometry(PassiveRef<Geometry3D> rGeom);
        bool splinesCreated(const std::vector<PassiveRef<Geometry3D> >& geoms, std::vector<bool>& created);

		/// Clean up data
		void cleanup();

		/// Create constraint (all kinds of concrete constraints)
		bool createConstraint(ISketchConstraint* pCons, bool bCheckInValidCons = false);

		bool setVCSConstraintData(ISketchConstraint* pCons);

		void addAdditionalConstraints();

		void dumpSolverData();
		void checkSolverResults();

		void getAssociatedConstraints(PassiveRef<MSketch> rSketch,
			std::vector<ISketchConstraint*> &associatedCons, 
			ISketchConstraint* pNewCons,
			const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag);
		void getAssociatedConstraintsItem(std::vector<ISketchConstraint*> &associatedCons, 
			std::vector< PassiveRef<Geometry3D> >& associatedGeoms,
			std::vector<ISketchConstraint*>& constraintList,
			PassiveRef<Geometry3D>& targetGeom);
		
		/// Get connected geometries (such as line/arc's end points, that not related with constraint)
		void getConnectedGeometries(const PassiveRef<Geometry3D>& targetGeom, 
			std::vector< PassiveRef<Geometry3D> >& alreadyAddedGeoms,
			std::vector< PassiveRef<Geometry3D> >& outputGeoms);
		void addGeomToList(const PassiveRef<Geometry3D>& rGeom,
			std::vector< PassiveRef<Geometry3D> >& listToCheckAdd,
			std::vector< PassiveRef<Geometry3D> >& listToAdd,
			std::vector< PassiveRef<Geometry3D> >& listToAdd2);
		/// Get target geometries related with input geometries to do solving
		bool getTargetGeomsForSolving(const std::vector< PassiveRef<Geometry3D> >& inputGeometries, 
			std::vector<ISketchConstraint*>& sketchAllCons,
			std::vector< PassiveRef<Geometry3D> >& targetGeoms);

		/// Do solving by check un-meet constraints, especially for cases when the operations affect multi constraints
		/// Such as updating multi projected geometries in polaris or sketch edit commands like fillet/extend.
		bool solveByCheckingUnmeetConstraints(ISketchConstraint* pCons);

		/// Active sketch to do solving
		PassiveRef<MSketch> m_rSketch;

		/// Map from geometry to solver geometry
		typedef std::map< PassiveRef<Geometry3D>, SketchSolverGeometry* > GeomToSolverGeomMap;
		GeomToSolverGeomMap m_geomToSolverGeomMap;

		/// VCS Interface
		SketchVCSInterface* m_pVCSInterface;

		/// Updated sketch points
		std::vector< PassiveRef<Point3D> > m_updatedPoints;
		std::vector< PassiveRef<Curve3D> > m_updatedCurves;

		/// Used to store constraint data
		SketchVCSConstraintData m_consData;

		/// Used to store dragging data
		SketchVCSDraggingData m_dragData;

		std::vector<VCSConHandle*> m_newAddedCons;
		/// Whether current is dragging
		bool m_bIsDragging;
		SketchVCSStatus* m_pStatus;
		AcGeTol m_tol;
	};
}}
