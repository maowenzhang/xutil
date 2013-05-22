#include "Pch/SketchPch.h"
#include "SketchVCSStatus.h"

#include <Server/Base/Assert/Assert.h>

using namespace SKs;
using namespace SKs::Constraint;

SketchVCSStatus::SketchVCSStatus()
	: m_eVCSStatus(kNull)
{
}

SketchVCSStatus::~SketchVCSStatus()
{

}

void SketchVCSStatus::status(VCSStatus& eStatus)
{
	m_eVCSStatus = eStatus;
}

VCSStatus& SketchVCSStatus::status()
{
	return m_eVCSStatus;
}

bool SketchVCSStatus::isSuccess() const
{
	if (kSolved == m_eVCSStatus || kSolvedWithoutDOFAndRedundancyAnalysis == m_eVCSStatus)
		return true;
	return false;
}

bool SketchVCSStatus::isSuccess(VCSStatus& eStatus)
{
	status(eStatus);
	return isSuccess();
}

const Ns::IString& SketchVCSStatus::errorMessage() const
{
	NEUTRON_ASSERT(!isSuccess());
	
	//static Ns::IString errMessageError = _LCLZ("SketchConstraintSolvingError", "Failed to do constraint solving!");
	static Ns::IString errMessageOverConstraint = _LCLZ("SketchConstraintSolvingOverConstraint", "Sketch geometry is over constrained");

	//if (kRedundant == m_eVCSStatus)
	return errMessageOverConstraint;
}

void SketchVCSStatus::cleanup()
{
	m_eVCSStatus = kNull;
}