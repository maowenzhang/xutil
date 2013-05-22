#include "Pch/SketchPch.h"
#include "SketchVCSExtRigidTester.h"
#include "SketchSolverGeometry.h"

using namespace SKs::Constraint;

/// ----------------------------------------------------------------------
/// SketchVCSExtRigidTester
/// ----------------------------------------------------------------------

SketchVCSExtRigidTester::SketchVCSExtRigidTester(SketchSolverGeometry* pSolverGeom)
	: m_pSolverGeom(pSolverGeom)
{
}

SketchVCSExtRigidTester::~SketchVCSExtRigidTester()
{
	m_pSolverGeom = NULL;
}

bool SketchVCSExtRigidTester::isExternalRigid()
{
	return m_pSolverGeom->isExternalRigid();
}