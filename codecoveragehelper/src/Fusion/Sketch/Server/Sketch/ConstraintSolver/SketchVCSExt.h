#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep VCS extension classes for sketch
/// ----------------------------------------------------------------------

// Define _MAC for including VCS headers
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <VCSAPI.h> // for VCSExtVerificationCon

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;

	/// This class is used to represent VCSExtRigidTester used in VCS solving. 
	/// 
	class SketchVCSExtVerificationCon : public VCSExtVerificationCon
	{
	public:
		SketchVCSExtVerificationCon(SketchConstraintSolverContext* pContext);
		virtual ~SketchVCSExtVerificationCon();

		/// This virtual function is called when VCS needs to query the rigidity of the external geometry.
		virtual bool isRelationMet();

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSExtVerificationCon(const SketchVCSExtVerificationCon&);
		SketchVCSExtVerificationCon& operator=(const SketchVCSExtVerificationCon&);

		SketchConstraintSolverContext* m_pContext;
	};
}}
