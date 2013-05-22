#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSConstraintData
/// ----------------------------------------------------------------------
#include "SketchSolverGeometry.h" // for ESolverGeometryType
#include "../Objects/SketchConstraintType.h"

#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <vector>
#include <map>

class VCSConHandle;
class VCSPatternHandle;

namespace Ns { namespace Comp {
	class VCSInterface;
}}

namespace SKs {
	class ISketchConstraint;
}

using namespace Ns::Comp;

namespace SKs { namespace Constraint {
	
	class SketchSolverGeometry;
	class SketchConstraintSolverContext;
	class SketchSolverLine;
	class SketchSolverCircArc;
	class SketchSolverPoint;
	class SketchVCSConstraintData;
	class SketchVCSConstraintDataItem;
	class SketchVCSDraggingData;

	typedef std::vector<SketchVCSConstraintDataItem*> SolverGeomConsList;
	typedef std::map<ISketchConstraint*, SolverGeomConsList> SketchToVCSConstraintMap;

	/// This is used to get build up relationship between sketch constraint and vcs constraints
	///
	class SketchVCSConstraintMapHelper
	{
	public:
		SketchVCSConstraintMapHelper(ISketchConstraint* pSketchCons, SketchVCSConstraintData* pData);
		~SketchVCSConstraintMapHelper();

	private:
		ISketchConstraint* m_pSketchCons;
		SketchVCSConstraintData* m_pData;
		size_t m_index;
	};

	/// This is used to store data for single geometry
	///
	class SketchVCSConstraintDataItem
	{
	public:
		SketchVCSConstraintDataItem();
		SketchVCSConstraintDataItem(const std::vector<SketchSolverGeometry*>& geoms,
			VCSConHandle* pConsHandle,
			double dConsValue = 0.0,
			SketchConstraintType eConsType = eNone,
			bool bIsVariableCons = false,
			bool bTempAddedConstraint = false);
		SketchVCSConstraintDataItem(const SketchVCSConstraintDataItem& rhs);
		SketchVCSConstraintDataItem& operator=(const SketchVCSConstraintDataItem& rhs);

		~SketchVCSConstraintDataItem();

		SketchSolverGeometry* geom1() const;
		void geom1(SketchSolverGeometry* pGeom);
		SketchSolverGeometry* geom2() const;
		void geom2(SketchSolverGeometry* pGeom);
		const std::vector<SketchSolverGeometry*>& geoms() const;
		bool isContains(SketchSolverGeometry* pGeom1, SketchSolverGeometry* pGeom2) const;
		bool containsGeom(SketchSolverGeometry* pGeom) const;

		VCSConHandle* consHandle();
		void consHandle(VCSConHandle* pHandle, bool isRelationMeet);
		bool isRelationMeet() const;
		double consValue() const;
		void consValue(double val);
		void patternHandle(VCSPatternHandle* pHandle);
		VCSPatternHandle* patternHandle();

		void dumpSolverData(std::vector<Ns::IString>& strData);
		bool isTempAddedConstraint() const;
		bool isVariableCons() const;

		bool isMinimumValueCons() const;
		void isMinimumValueCons(bool val);

		SketchConstraintType consType() const;
		void consType(SketchConstraintType val);

		void biasPt(const AcGePoint3d& biasPt);
		const AcGePoint3d& biasPt() const;

	private:
		std::vector<SketchSolverGeometry*> m_geoms;
		VCSConHandle*				m_pConsHandle;
		double						m_dConsValue;
		bool						m_bTempAddedConstraint;
		/// Indicate whether this is minimum value constraint
		bool						m_bIsMinimumValueCons;
		SketchConstraintType        m_eConsType;
		bool						m_bIsVariableCons;
		AcGePoint3d					m_biasPt;
		bool						m_bIsRelationMeet;
		VCSPatternHandle*			m_pPatternHandle;
	};

	/// This is used to store data for creating VCS constraint
	/// 
	class SketchVCSConstraintData
	{
	public:
		SketchVCSConstraintData();
		SketchVCSConstraintData(SketchSolverGeometry* pGeom1, SketchSolverGeometry* pGeom2);
		~SketchVCSConstraintData();

		SketchSolverGeometry* geom1() const;
		SketchSolverGeometry* geom2() const;
		void geom1(SketchSolverGeometry* pGeom);
		void geom2(SketchSolverGeometry* pGeom);
		void consValue(double dVal);
		double consValue() const;
		bool hasInvalidConstraint() const;
		void hasInvalidConstraint(bool val);
		
		bool isAddingNewConstraint() const;
		void isAddingNewConstraint(bool val);

		SketchConstraintType consType() const;

		/// Get new added constraint (added last time)
		void newAddedConsHandle(std::vector<VCSConHandle*>& newAddedCons);
		/// Add new constraint
		void addConsHandle(VCSConHandle* pConsHandle, bool isRelationMeet = true);
		void addPatternHandle(VCSPatternHandle* pHandle);
		/// Get all constraints
		std::vector<VCSConHandle*>& consHandles();

		int geomCount();
		bool hasGeom1() const;
		bool hasGeom1And2() const;
		bool checkGeomTypes(ESolverGeometryType geomType);
		bool checkGeomTypes(ESolverGeometryType geom1Type, ESolverGeometryType geom2Type);
		/// Check all geometries are the same type
		bool checkGeomTypesAll(ESolverGeometryType geomType);

		void resetData(SketchSolverGeometry* pGeom1, 
			SketchSolverGeometry* pGeom2 = NULL, 
			double dConsValue = 0.0, 
			SketchConstraintType eConsType = eNone,
			bool bIsVariableCons = false,
			bool bTempAddConstraint = false);
		
		void resetData(const std::vector<SketchSolverGeometry*>& geoms, 
			double dConsValue = 0.0, 
			SketchConstraintType eConsType = eNone,
			bool bIsVariableCons = false,
			bool bTempAddConstraint = false);

		/// Use the old/previous cached constraint type
		void resetDataEX(SketchSolverGeometry* pGeom1, 
			SketchSolverGeometry* pGeom2 = NULL, 
			double dConsValue = 0.0,
			bool bIsVariableCons = false);

		void cleanup();

		/// Get how many related constraints on specified solverGeometry
		int relatedConstraintNum(SketchSolverGeometry* pSolverGeom);

		void dumpSolverData(std::vector<Ns::IString>& strData);

		/// Restore cached data
		void restoreData();
		/// Revert temporary changes
		void revertTempChanges(SketchConstraintSolverContext& ctx);

		SketchVCSConstraintDataItem& curConsItem();

		/// Check whether has related constraint between two solver geometry
		bool getRelatedLinesFromPoint(SketchSolverGeometry* pPoint, std::vector<SketchSolverLine*>& relatedLines);
		bool hasRelatedConsWithLinePreventFixEndPoint(SketchSolverGeometry* pLine);
		/// Check whether has related constraint on circle, to make the circle as variable
		bool hasRelatedConsWithCircleToMakeVariable(SketchSolverGeometry* pCircle, bool bDragCenterPoint = false);
		/// When solving (add new constraint), check whether has related constraint on circle to make it as variable
		bool hasRelatedConsWithCircleToMakeVariableInSolving(SketchSolverGeometry* pCircle);

		/// Check whether have constraint between two geometries
		bool hasRelatedConsBetweenGeometry(SketchSolverGeometry* pGeom1, SketchSolverGeometry* pGeom2, bool bIsExcludeVarCons = true);
		/// Handle for symmetry constraint when dragging
		bool handleSymmetryConsWhenDragging(SketchSolverGeometry* pGeom, bool isDragRelatedItem = false);
		/// Handle polygon constraint when dragging
		bool handlePolygonConsWhenDragging(SketchSolverGeometry* pPoint, SketchVCSDraggingData& dragData);
        // if a handle is paralled with another handle, drag the start point of this handle, other handle's fit point will float
        // we will fix another handle's fit point too
		void handleConstrainedHandleWhenDragging(SketchSolverGeometry* pLine, SketchConstraintSolverContext* ctx);

		void getUnmeetConsHandle(SketchToVCSConstraintMap& unmeetCons);	

		bool shouldFixCircleArcEndPoint(SketchSolverGeometry* pCirArc, SketchSolverGeometry* pEndPt);

		bool shouldFixLineEndPoint(SketchSolverLine* pLine, SketchSolverGeometry* pEndPt, SketchSolverGeometry* pOtherPt);

		bool addTempGroundGeom(SketchSolverGeometry* pGeom, int iSolveCount = 1);
		bool tempGroundGeomesPreSolve(ISketchConstraint* pSketchCons, SketchConstraintSolverContext& ctx);
		bool doesNeedSolveAgain(int iSolveCount);
		
		ISketchConstraint* newAddConstraint();
		void newAddConstraint(ISketchConstraint* pNewAddCons);

		/// Change geometries order, to suit solve behavior
		/// Such as: when edit dimension, last created item should be moved
		bool preferMovingLaterCreatedGeom();

		void addCoincidentPointsToDrag(SketchSolverGeometry* pGeom, SketchVCSDraggingData& dragData);

		bool hasSymmetryConstraintOnGeom(SketchSolverGeometry* pGeom);

	private:
		friend class SketchVCSConstraintMapHelper;

		/// Current we don't need copy and assignment constructors
		SketchVCSConstraintData(const SketchVCSConstraintData&);
		SketchVCSConstraintData& operator=(const SketchVCSConstraintData&);

		void addToSolverGeomConsMap(SketchSolverGeometry* pSolverGeom);
		/// Pick one item to ground, current used for symmetry constraint
		void pickGeometryToGround(SketchSolverGeometry* pGeom1, 
			SketchSolverGeometry* pGeom2, 
			SketchSolverGeometry* pBaseLine);

		void addToConstraintsMap(ISketchConstraint* pSketchCons, size_t consIndex);
		
		bool setTempGroundGeom(bool isSetGround, int iSolveCount);		

		SolverGeomConsList m_solverGeomConsList;	
		SketchVCSConstraintDataItem* m_pCachedConsItem;
		SketchVCSConstraintDataItem  m_curConsItem;
		/// Indicate whether have invalid constraint (that need to do solve except the new added constraint)
		bool m_bHasInvalidConstraint;
		
		SketchToVCSConstraintMap m_sketchToVCSConsMap;
		bool m_bIsAddingNewConstraint;

		// Store geometries that we try to ground, and un-ground if solve fails
		std::vector<SketchSolverGeoms> m_tempGroundedGeoms;
		ISketchConstraint* m_pNewAddConstraint;
	};
}}
