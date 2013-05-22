#include "Pch/SketchPch.h"

#define __COMPILING_SKS_SKETCHCONSTRAINTSOLVER_CPP__
#include "SketchConstraintSolver.h"
#undef __COMPILING_SKS_SKETCHCONSTRAINTSOLVER_CPP__

#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSStatus.h"
#include "../Objects/ISketchConstraint.h"

using namespace SKs;
using namespace SKs::Constraint;
using namespace SKs::Geometry;

SketchConstraintSolver::SketchConstraintSolver()
{
	m_pSolverContext = new SketchConstraintSolverContext();
}

SketchConstraintSolver::~SketchConstraintSolver()
{
	if (NULL != m_pSolverContext)
	{
		delete m_pSolverContext;
		m_pSolverContext = NULL;
	}
}

bool SketchConstraintSolver::solve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons)
{
	return m_pSolverContext->solve(rSketch, pCons);
}

bool SketchConstraintSolver::dragBegin(PassiveRef<MSketch> rSketch, 
	const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag,
	const AcGePoint3d& startPt)
{
	bool res = m_pSolverContext->dragBegin(rSketch, geomtriesToDrag, startPt);
	NEUTRON_ASSERT(res);
	return res;
}

bool SketchConstraintSolver::drag(AcGePoint3d& oldPos, const AcGePoint3d& newPos, const AcGeVector3d& viewVec)
{
	bool res = m_pSolverContext->drag(oldPos, newPos, viewVec);
	return res;
}

void SketchConstraintSolver::getChangedGeometries(std::vector< PassiveRef<Geometry3D> >& gemosChanged)
{
	m_pSolverContext->getChangedGeometries(gemosChanged);
}

const Ns::IString& SketchConstraintSolver::errorMessage() const
{
	return m_pSolverContext->status()->errorMessage();
}