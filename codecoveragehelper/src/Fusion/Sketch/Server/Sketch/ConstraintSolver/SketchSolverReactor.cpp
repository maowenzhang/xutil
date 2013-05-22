#include "Pch/SketchPch.h"
#include "SketchSolverReactor.h"
#include "SketchSolverGeometry.h"

using namespace SKs::Constraint;

/// ----------------------------------------------------------------------
/// SketchSolverReactor
/// ----------------------------------------------------------------------

SketchSolverReactor::SketchSolverReactor(SketchSolverGeometry* pSolverGeom, EReactorType eType)
	: m_pSolverGeom(pSolverGeom),
	m_bRigid(false),
	m_eType(eType)
{
}

SketchSolverReactor::~SketchSolverReactor()
{
	m_pSolverGeom = NULL;
	m_bRigid = false;
	m_eType = eNone;
}

int SketchSolverReactor::notify(VCSMessage x)
{
	if (kInitialize == x)
		m_bRigid = false;
	if (kFullySolved == x || kFullySolvedChanged == x)
		m_bRigid = true;
	return m_pSolverGeom->onReactorNotify((int)x);
}

bool SketchSolverReactor::isRigid()
{
	return m_bRigid;
}

SketchSolverReactor::EReactorType SketchSolverReactor::reactorType()
{
	return m_eType;
}
