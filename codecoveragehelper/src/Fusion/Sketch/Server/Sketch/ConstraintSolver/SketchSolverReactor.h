#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchSolverGeometry (point, line, circle, etc)
/// ----------------------------------------------------------------------

// Define _MAC for including VCS headers
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <VCSAPI.h> // for VCSBodyReactor

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;

	/// This class is used to represent VCS body reactor used in VCS solving. 
	/// Used to get notification from bodies, attach to the (solver geometry) body 
	/// it has interest in to get the notification from the body.
	/// 
	class SketchSolverReactor : public VCSBodyReactor
	{
	public:
		enum EReactorType
		{
			eNone,
			eSplineFitPoint,
			eSpline
		};

		SketchSolverReactor(SketchSolverGeometry* pSolverGeom, EReactorType eType = eNone);
		virtual ~SketchSolverReactor();

		/// the notification sent by the objects that this reactor is attached to
		virtual int notify(VCSMessage x);

		/// Check whether it's rigid
		bool isRigid();

		EReactorType reactorType();

	private:
		/// Current we don't need copy and assignment constructors
		SketchSolverReactor(const SketchSolverReactor&);
		SketchSolverReactor& operator=(const SketchSolverReactor&);

		SketchSolverGeometry* m_pSolverGeom;
		bool m_bRigid;
		EReactorType m_eType;
	};
}}
