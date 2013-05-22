#include "Pch/SketchPch.h"
#include "SketchVCSExt.h"
#include "SketchConstraintSolverContext.h"

using namespace SKs::Constraint;

/// ----------------------------------------------------------------------
/// SketchVCSExtVerificationCon
/// ----------------------------------------------------------------------

SketchVCSExtVerificationCon::SketchVCSExtVerificationCon(SketchConstraintSolverContext* pContext)
	: m_pContext(pContext)
{
}

SketchVCSExtVerificationCon::~SketchVCSExtVerificationCon()
{
	m_pContext = NULL;
}

bool SketchVCSExtVerificationCon::isRelationMet()
{
	return m_pContext->isRelationMet();
}