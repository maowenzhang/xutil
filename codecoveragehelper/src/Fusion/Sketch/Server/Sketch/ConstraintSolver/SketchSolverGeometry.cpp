#include "Pch/SketchPch.h"
#include "SketchSolverGeometry.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"
#include "SketchSolverReactor.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverGeometry
/// ----------------------------------------------------------------------

AcGeLine3d SketchSolverGeometry::m_defaultLine = AcGeLine3d::kXAxis;
AcGeCircArc3d SketchSolverGeometry::m_defaultArc = AcGeCircArc3d();
AcGeEllipArc3d SketchSolverGeometry::m_defaultEllipArc = AcGeEllipArc3d();

SketchSolverGeometry::SketchSolverGeometry(SketchConstraintSolverContext* pContext, ESolverGeometryType geomType)
	: m_pContext(pContext),
	m_geomType(geomType),
	m_bAddedAdditionalConstraints(false),
	m_bUpdatedBefore(false)
{
	NEUTRON_ASSERT(NULL != m_pContext);
	m_pVcsRigidBody = pContext->vcsInterface()->createRBody();
}

SketchSolverGeometry::~SketchSolverGeometry()
{
	m_pContext = NULL;
	m_pVcsRigidBody = NULL;
}

bool SketchSolverGeometry::updateValueBySolvedBody(bool bUpdateToLastSolved)
{
	if (bUpdateToLastSolved)
		return updateValueToLastSolved();

	NEUTRON_ASSERT(NULL != vcsBody());
	if (NULL == vcsBody())
		return false;
	
	VCSMMatrix3d vMat = vcsBody()->transform();
	AcGeMatrix3d aMat = GeConv::toAcGeMatrix3d(vMat);
	if (aMat.isEqualTo(AcGeMatrix3d::kIdentity))
	{
		// Update cached value if the geometry is updated before
		// It means moving back to original position, so we should update the cached value to it (FUS-4316)
		if (m_bUpdatedBefore)
			updateCachedValue();

		// For solverGeomCircArc, we need to check the varGeomHandle as the radius might be changed
		// For solverGeomEllipse, need to check as the radius and axis might be changed
		if (geomType() != eSolverGeomCirArc && geomType() != eSolverGeomEllipticArc)
			return true;
	}

	m_bUpdatedBefore = true;
	return updateValue(aMat);
}

bool SketchSolverGeometry::checkPreUpdateValue()
{
	return true;
}

bool SketchSolverGeometry::addAdditionalConstraints(SketchVCSConstraintData& data)
{
	if (m_bAddedAdditionalConstraints)
		return true;

	m_bAddedAdditionalConstraints = true;

	bool res = onAddAdditionalConstraints(data);
	return res;
}

bool SketchSolverGeometry::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
	dragData.addBodyToDrag(vcsBody());
	return true;
}

void SketchSolverGeometry::addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c)
{
}

/// Add extra geometries for dragging
bool SketchSolverGeometry::addExtraGeomsForDragging(SketchVCSDraggingData& dragData)
{
	return true;
}

bool SketchSolverGeometry::addExtraHandlesForDragging(SketchVCSDraggingData& dragData)
{
	return true;
}

void SketchSolverGeometry::initialize()
{
	//
}

ESolverGeometryType SketchSolverGeometry::geomType() const
{
	return m_geomType;
}

VCSRigidBody* SketchSolverGeometry::vcsBody()
{
	return m_pVcsRigidBody;
}

const AcGePoint3d& SketchSolverGeometry::position()
{
	return m_position;
}

void SketchSolverGeometry::position(const AcGePoint3d& pos)
{
	m_position = pos;
}

VCSMPoint3d SketchSolverGeometry::positionEx()
{
	const AcGePoint3d& gePt = position();
	return GeConv::toVCSMPoint3d(gePt);
}

VCSMLine3d SketchSolverGeometry::lineEx()
{
	const AcGeLine3d& geLine = line();
	return GeConv::toVCSMLine3d(geLine);
}

const AcGeLine3d& SketchSolverGeometry::line()
{
	NEUTRON_ASSERT(false);
	return m_defaultLine;
}

AcGeVector3d SketchSolverGeometry::lineDirection()
{
	return m_defaultLine.direction();
}

const AcGeCircArc3d& SketchSolverGeometry::circArc() const
{
	NEUTRON_ASSERT(false);
	return m_defaultArc;
}


const AcGeEllipArc3d& SketchSolverGeometry::ellipArc() const
{
	NEUTRON_ASSERT(false);
	return m_defaultEllipArc;
}

SketchSolverGeometry* SketchSolverGeometry::centerPointSolverGeometry()
{
	NEUTRON_ASSERT(false);
	return NULL;
}

void SketchSolverGeometry::setGrounded(bool bGrounded)
{
	m_pVcsRigidBody->setGrounded(bGrounded);
}

bool SketchSolverGeometry::isGrounded()
{
	return vcsBody()->isGrounded();
}

Ns::IString SketchSolverGeometry::dumpInfo()
{
	IString info;
	
	// Get type string
	IString strType;
	switch (m_geomType)
	{
	case eSolverGeomPoint:
		strType = _DNGI("Point");
		break;
	case eSolverGeomLine:
		strType = _DNGI("Line");
		break;
	case eSolverGeomCirArc:
		strType = _DNGI("CirArc");
		break;
	case eSolverGeomEllipticArc:
		strType = _DNGI("EllipticArc");
		break;
	case eSolverGeomSpline:
		strType = _DNGI("Spline");
		break;
	}
	info += strType;
	int id = static_cast<int>(vcsBody()->id());
	info += (boost::wformat(_DNGI("%1% :")) %id).str();

	if (vcsBody()->isGrounded())
		info += _DNGI(" FixedBody");

	return info;
}

bool SketchSolverGeometry::addOnPlaneConstraint(SketchVCSConstraintData& data)
{
	return false;
}

int SketchSolverGeometry::onReactorNotify(int iVCSMessage)
{
	NEUTRON_ASSERT(false);
	return 0;
}

/// Get extension curve
SketchVCSExtCurve* SketchSolverGeometry::extCurve()
{
	NEUTRON_ASSERT(false);
	return NULL;
}

bool SketchSolverGeometry::isExternalRigid()
{
	NEUTRON_ASSERT(false);
	return true;
}

/// Get rigid tester
VCSExtGeometry* SketchSolverGeometry::extGeometry()
{
	NEUTRON_ASSERT(false);
	return NULL;
}

VCSVarGeomHandle* SketchSolverGeometry::varGeomHandle()
{
	NEUTRON_ASSERT(false);
	return NULL;
}

AcGePoint3d SketchSolverGeometry::updatedPosition()
{
	AcGePoint3d pt = position();

	VCSMMatrix3d vMat = vcsBody()->transform();
	AcGeMatrix3d aMat = GeConv::toAcGeMatrix3d(vMat);
	if (!aMat.isEqualTo(AcGeMatrix3d::kIdentity))
		pt.transformBy(aMat);

	return pt;
}

bool SketchSolverGeometry::addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem)
{
	return false;
}

void SketchSolverGeometry::addReactor()
{
	NEUTRON_ASSERT(false);
}

bool SketchSolverGeometry::updateValueToLastSolved()
{
	return true;
}

bool SketchSolverGeometry::useCombinedBody() const
{
	return false;
}

void SketchSolverGeometry::useCombinedBody(bool val)
{
	NEUTRON_ASSERT(false);
}

void SketchSolverGeometry::useCombinedBody(SketchSolverGeometry* pGeom)
{
	NEUTRON_ASSERT(false);
}

void SketchSolverGeometry::updateCachedValue()
{
	
}
