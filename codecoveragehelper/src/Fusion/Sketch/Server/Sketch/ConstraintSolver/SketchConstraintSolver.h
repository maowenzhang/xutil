#pragma once

#include <Server/Archive/Globals.h>
#include <Server/DataModel/Refs/PassiveRef.h>
#include <vector>

#ifdef MSKETCH_EXPORTS
# ifdef __COMPILING_SKS_SKETCHCONSTRAINTSOLVER_CPP__
#  define MSKETCH_API NS_EXPORT
# else
#  define MSKETCH_API
# endif
#else
# define MSKETCH_API NS_IMPORT
#endif

/// ----------------------------------------------------------------------
/// This file is used to keep SketchConstraintSolver
/// ----------------------------------------------------------------------

namespace SKs {
	class ISketchConstraint;
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
using namespace SKs;
using namespace SKs::Geometry;

namespace SKs { namespace Constraint {
	
	class SketchConstraintSolverContext;

	/// This is the unique entry point of sketch constraint solver system. 
	/// The detail constraint works are delegated to related context class (SketchConstraintSolverContext), 
	/// the context class leverages VCS APIs via SketchVCSInterface.
	class MSKETCH_API SketchConstraintSolver
	{
	public:
		SketchConstraintSolver();
		~SketchConstraintSolver();

		/// Initialization, such as loading constraint data from sketch
		bool initialize(PassiveRef<MSketch> rSketch);

		/// Do Solve
		bool solve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons = NULL);

		/// On Drag begin, do some initialization
		bool dragBegin(PassiveRef<MSketch> rSketch, const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag, const AcGePoint3d& startPt);

		/// Do Drag, on mouse move
		bool drag(AcGePoint3d& oldPos, const AcGePoint3d& newPos, const AcGeVector3d& viewVec);

		/// Get the changed geometries during drag
		void getChangedGeometries(std::vector< PassiveRef<Geometry3D> >& gemosChanged);
		/// Get error message
		const Ns::IString& errorMessage() const;
		
	private:
		/// Current we don't need copy and assignment constructors
		SketchConstraintSolver(const SketchConstraintSolver&);
		SketchConstraintSolver& operator=(const SketchConstraintSolver&);

		/// Context object used to handle some data and implementation details
		SketchConstraintSolverContext* m_pSolverContext;
	};
}}

#undef MSKETCH_API