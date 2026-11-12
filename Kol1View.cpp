// Kol1View.cpp : implementation of the CKol1View class  (I kolokvijum 2017 - AT-AT)
#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "Kol1.h"
#endif

#include "Kol1Doc.h"
#include "Kol1View.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BODY_X 100.0f      // 3. gornji levi ugao tela
#define BODY_Y 100.0f
#define LEG_DY 168.0       // noge spustene u odnosu na telo
#define REAR_DX 72.0       // zadnje noge
#define FRONT_DX 270.0     // prednje noge

#define LEG1_PIVOT 28.0f   // 4. centar rotacije gornjeg dela (od gornjeg levog ugla)
#define LEG2_PIVOT 29.0f   //    centar rotacije donjeg dela

#define STEP_MOVE 10.0     // 5. pomeraj udesno po koraku
#define BACK_STEP 200      // 7. pomeraj pozadine

// 5. ciklus stanja gornjeg dela noge: -20 -10 0 10 20 10 0 -10
static const double LEG_ANGLES[] = { -20.0, -10.0, 0.0, 10.0, 20.0, 10.0, 0.0, -10.0 };
static const int PHASE_COUNT = sizeof(LEG_ANGLES) / sizeof(LEG_ANGLES[0]);

IMPLEMENT_DYNCREATE(CKol1View, CView)

BEGIN_MESSAGE_MAP(CKol1View, CView)
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// ---------------------------------------------------------------
// 1. Ucitavanje slika u konstruktoru, dealokacija u destruktoru
// ---------------------------------------------------------------
CKol1View::CKol1View() noexcept
{
	m_pBody = new DImage();  m_pBody->Load(CString("Body.png"));
	m_pLeg1 = new DImage();  m_pLeg1->Load(CString("Leg1.png"));
	m_pLeg2 = new DImage();  m_pLeg2->Load(CString("Leg2.png"));
	m_pLeg3 = new DImage();  m_pLeg3->Load(CString("Leg3.png"));
	m_pBack = new DImage();  m_pBack->Load(CString("Back2.jpg"));

	// pocetno stanje kao na slici: zadnje noge 0, prednje +20 i -20
	m_legPhase[0] = 2;   // zadnja daleka   ->   0
	m_legPhase[1] = 6;   // zadnja bliza    ->   0
	m_legPhase[2] = 0;   // prednja daleka  -> -20
	m_legPhase[3] = 4;   // prednja bliza   -> +20

	m_moveX = 0.0;
	m_scale = 1.0;
	m_backX = 0;
	m_backY = 0;
}

CKol1View::~CKol1View()
{
	delete m_pBody;
	delete m_pLeg1;
	delete m_pLeg2;
	delete m_pLeg3;
	delete m_pBack;
}

BOOL CKol1View::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// ---------------------------------------------------------------
// 2. Transformacije
//    rightMultiply = false -> MWT_LEFTMULTIPLY  (lokalna, u odnosu na tekuci k.s.)
//    rightMultiply = true  -> MWT_RIGHTMULTIPLY (globalna)
// ---------------------------------------------------------------
void CKol1View::LoadIdentity(CDC* pDC)
{
	pDC->ModifyWorldTransform(NULL, MWT_IDENTITY);
}

void CKol1View::Translate(CDC* pDC, float dX, float dY, bool rightMultiply)
{
	XFORM form = { 1.0f, 0.0f, 0.0f, 1.0f, dX, dY };
	pDC->ModifyWorldTransform(&form, rightMultiply ? MWT_RIGHTMULTIPLY : MWT_LEFTMULTIPLY);
}

void CKol1View::Rotate(CDC* pDC, float angle, bool rightMultiply)
{
	float rad = (float)(angle * M_PI / 180.0);
	XFORM form = { cosf(rad), sinf(rad), -sinf(rad), cosf(rad), 0.0f, 0.0f };
	pDC->ModifyWorldTransform(&form, rightMultiply ? MWT_RIGHTMULTIPLY : MWT_LEFTMULTIPLY);
}

void CKol1View::Scale(CDC* pDC, float sX, float sY, bool rightMultiply)
{
	XFORM form = { sX, 0.0f, 0.0f, sY, 0.0f, 0.0f };
	pDC->ModifyWorldTransform(&form, rightMultiply ? MWT_RIGHTMULTIPLY : MWT_LEFTMULTIPLY);
}

// Transparentno crtanje - transparentna je boja prvog piksela
void CKol1View::DrawImgTransparent(CDC* pDC, DImage* pImage)
{
	unsigned char* bits = pImage->GetDIBBits();
	COLORREF bgColor = RGB(bits[2], bits[1], bits[0]);   // u memoriji je BGR(A)

	CRect rc(0, 0, pImage->Width(), pImage->Height());
	pImage->DrawTransparent(pDC, rc, rc, bgColor);
}

// ---------------------------------------------------------------
// 4. Jedna noga. Ulaz: k.s. tela (gornji levi ugao Body.png).
//    Leg1 se rotira za alpha oko (28,28); Leg2 i Leg3 ostaju vertikalni,
//    pa se na zglobu rotacija ponistava sa -alpha.
// ---------------------------------------------------------------
void CKol1View::DrawLeg(CDC* pDC, double alpha, double dx, double dy)
{
	XFORM oldForm;
	pDC->GetWorldTransform(&oldForm);

	// gornji deo (Leg1)
	Translate(pDC, (float)dx, (float)dy, false);           // gornji levi ugao noge
	Translate(pDC, LEG1_PIVOT, LEG1_PIVOT, false);         // centar rotacije
	Rotate(pDC, (float)alpha, false);
	Translate(pDC, -LEG1_PIVOT, -LEG1_PIVOT, false);
	DrawImgTransparent(pDC, m_pLeg1);

	// zglob na dnu Leg1 (simetrican gornjem: (28, h1 - 28))
	Translate(pDC, LEG1_PIVOT, m_pLeg1->Height() - LEG1_PIVOT, false);
	Rotate(pDC, (float)-alpha, false);                     // donji delovi vertikalni

	// donji deo (Leg2), njegov centar rotacije (29,29) je na zglobu
	Translate(pDC, -LEG2_PIVOT, -LEG2_PIVOT, false);
	DrawImgTransparent(pDC, m_pLeg2);

	// stopalo (Leg3) ispod Leg2, centrirano po X
	Translate(pDC, (m_pLeg2->Width() - m_pLeg3->Width()) / 2.0f,
		(float)m_pLeg2->Height(), false);
	DrawImgTransparent(pDC, m_pLeg3);

	pDC->SetWorldTransform(&oldForm);
}

// ---------------------------------------------------------------
// 3. Scena: pozadina, 4 noge i telo
// ---------------------------------------------------------------
void CKol1View::DrawScene(CDC* pDC, CRect rect)
{
	// pozadina (deo slike od (m_backX, m_backY) velicine prozora)
	int w = min(rect.Width(), m_pBack->Width());
	int h = min(rect.Height(), m_pBack->Height());
	m_pBack->Draw(pDC, CRect(m_backX, m_backY, m_backX + w, m_backY + h),
		CRect(rect.left, rect.top, rect.left + w, rect.top + h));

	int oldMode = pDC->SetGraphicsMode(GM_ADVANCED);
	XFORM oldForm;
	pDC->GetWorldTransform(&oldForm);

	// k.s. tela: (100,100) + kretanje; skaliranje oko gornjeg levog ugla tela
	Translate(pDC, BODY_X + (float)m_moveX, BODY_Y, false);
	Scale(pDC, (float)m_scale, (float)m_scale, false);

	// dalje noge (iza), pa bliže noge, pa telo preko gornjih zglobova
	DrawLeg(pDC, LEG_ANGLES[m_legPhase[0]], REAR_DX, LEG_DY);
	DrawLeg(pDC, LEG_ANGLES[m_legPhase[2]], FRONT_DX, LEG_DY);
	DrawLeg(pDC, LEG_ANGLES[m_legPhase[1]], REAR_DX, LEG_DY);
	DrawLeg(pDC, LEG_ANGLES[m_legPhase[3]], FRONT_DX, LEG_DY);
	DrawImgTransparent(pDC, m_pBody);

	LoadIdentity(pDC);
	pDC->SetWorldTransform(&oldForm);
	pDC->SetGraphicsMode(oldMode);
}

// ---------------------------------------------------------------
// 8. Crtanje bez flicker-a (memorijski DC)
// ---------------------------------------------------------------
BOOL CKol1View::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CKol1View::OnDraw(CDC* pDC)
{
	CKol1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rcClient;
	GetClientRect(&rcClient);

	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(pDC, rcClient.Width(), rcClient.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&bmp);

	memDC.FillSolidRect(rcClient, RGB(255, 255, 255));
	DrawScene(&memDC, rcClient);

	pDC->BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
	bmp.DeleteObject();
	memDC.DeleteDC();
}

// ---------------------------------------------------------------
// 5. -> korak udesno, 6. gore/dole skaliranje, 7. A/D/W/S pozadina
// ---------------------------------------------------------------
void CKol1View::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int maxBackX = max(0, m_pBack->Width() - rcClient.Width());
	int maxBackY = max(0, m_pBack->Height() - rcClient.Height());

	switch (nChar)
	{
	case VK_RIGHT:
		for (int i = 0; i < LEG_COUNT; i++)
			m_legPhase[i] = (m_legPhase[i] + 1) % PHASE_COUNT;
		m_moveX += STEP_MOVE;
		break;

	case VK_UP:   m_scale *= 1.1; break;
	case VK_DOWN: m_scale /= 1.1; break;

	// pozadina: A/W ne rade ako je leva/gornja ivica slike uz ivicu prozora,
	// D/S ne rade ako je desna/donja ivica slike uz ivicu prozora
	case 'A': m_backX = max(0, m_backX - BACK_STEP); break;
	case 'D': m_backX = min(maxBackX, m_backX + BACK_STEP); break;
	case 'W': m_backY = max(0, m_backY - BACK_STEP); break;
	case 'S': m_backY = min(maxBackY, m_backY + BACK_STEP); break;

	default:
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	Invalidate();
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
