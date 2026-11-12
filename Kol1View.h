// Kol1View.h : interface of the CKol1View class  (I kolokvijum 2017 - AT-AT)
#pragma once
#include "DImage.h"

#define LEG_COUNT 4

class CKol1View : public CView
{
protected:
	CKol1View() noexcept;
	DECLARE_DYNCREATE(CKol1View)

public:
	CKol1Doc* GetDocument() const;

	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ~CKol1View();

protected:
	// 1. slike
	DImage* m_pBody;
	DImage* m_pLeg1;
	DImage* m_pLeg2;
	DImage* m_pLeg3;
	DImage* m_pBack;

	// 5. faza svake noge (indeks u nizu uglova) i pomeraj transportera
	int m_legPhase[LEG_COUNT];
	double m_moveX;

	// 6. faktor skaliranja
	double m_scale;

	// 7. pomeraj pozadine (gornji levi ugao dela slike koji se vidi)
	int m_backX;
	int m_backY;

	// 2.
	void LoadIdentity(CDC* pDC);
	void Translate(CDC* pDC, float dX, float dY, bool rightMultiply);
	void Rotate(CDC* pDC, float angle, bool rightMultiply);   // angle u stepenima
	void Scale(CDC* pDC, float sX, float sY, bool rightMultiply);

	// 3., 4.
	void DrawScene(CDC* pDC, CRect rect);
	void DrawLeg(CDC* pDC, double alpha, double dx, double dy);

	void DrawImgTransparent(CDC* pDC, DImage* pImage);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

#ifndef _DEBUG
inline CKol1Doc* CKol1View::GetDocument() const
   { return reinterpret_cast<CKol1Doc*>(m_pDocument); }
#endif
