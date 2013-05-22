#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSExtRigidTester
/// ----------------------------------------------------------------------

// Define _MAC for including VCS headers
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <VCSExt.h> // for VCSExtRigidTester

class VCSMPoint3d;

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;

	/// This class is used to represent VCSExtRigidTester used in VCS solving. 
	/// 
	class SketchVCSExtRigidTester : public VCSExtRigidTester
	{
	public:
		SketchVCSExtRigidTester(SketchSolverGeometry* pSolverGeom);
		virtual ~SketchVCSExtRigidTester();

		/// This virtual function is called when VCS needs to query the rigidity of the external geometry.
		virtual bool isExternalRigid();

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSExtRigidTester(const SketchVCSExtRigidTester&);
		SketchVCSExtRigidTester& operator=(const SketchVCSExtRigidTester&);

		SketchSolverGeometry* m_pSolverGeom;
	};
}}
