#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSDraggingData
/// ----------------------------------------------------------------------
#include "SketchSolverGeometry.h" // for ESolverGeometryType

#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <Server/DataModel/Refs/PassiveRef.h>
#include <vector>
#include <gepnt3d.h>
#include <VCSColl.h> // VCSCollection

class VCSRigidBody;

namespace Ns { namespace Geometry {
	class Geometry3D;
}}

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;

	/// This is used to store data for creating VCS constraint
	/// 
	class SketchVCSDraggingData
	{
	public:
		SketchVCSDraggingData();
		~SketchVCSDraggingData();

		void cleanup();
		void resetData(const AcGePoint3d& startPt);
		VCSCollection& bodiesToDrag();
		const AcGePoint3d& startPt() const;
		bool IsDragSingleGeom() const;
		void addBodyToDrag(VCSRigidBody* pVCSBody);
		void addGeomToDrag(SketchSolverGeometry* pGeom);

		/// Check whether the input geometry is being dragged
		bool isDragGeom(SketchSolverGeometry* pGeom) const;

		std::vector<SketchSolverGeometry*>& geometriesToDrag();

		void dumpSolverData(Ns::IString& strData);

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSDraggingData(const SketchVCSDraggingData&);
		SketchVCSDraggingData& operator=(const SketchVCSDraggingData&);

		AcGePoint3d m_startPt;
		/// Bodies to drag
		VCSCollection m_bodiesToDrag;
		std::vector<SketchSolverGeometry*> m_geomtriesToDrag;
	};
}}
