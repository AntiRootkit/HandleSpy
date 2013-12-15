#include "StdAfx.h"
#include "ShionChartView.h"

CShionChartView::CShionChartView(void)
{
	m_pVec = NULL;
	m_pChartLinePen = NULL;
	m_pSelectLinePen = NULL;
}

CShionChartView::~CShionChartView(void)
{
	m_pVec = NULL;
	
	if (m_pChartLinePen)
	{
		delete m_pChartLinePen;
		m_pChartLinePen = NULL;
	}
	
	if (m_pSelectLinePen)
	{
		delete m_pSelectLinePen;
		m_pSelectLinePen = NULL;
	}

	if (m_pSelectRectBrush)
	{
		delete m_pSelectRectBrush;
		m_pSelectRectBrush = NULL;
	}
}

BOOL CShionChartView::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return FALSE;
}

LRESULT CShionChartView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_pChartLinePen = new Pen(Color(255, 255, 0, 0), 1);
	m_pSelectLinePen = new Pen(Color(150, 0, 0, 255), 2);
	m_pSelectRectBrush = new SolidBrush(Color(50, 0, 0, 255));

	ResetLineChart();

	m_ToolTip.Create(m_hWnd);
	m_ToolTip.AddTool(m_hWnd);
	m_ToolTip.SetMaxTipWidth(_MAX_PATH);

	return 0;
}

void CShionChartView::ResetLineChart()
{
	m_pVec = NULL;

	m_dwMaxCount = 0;
	m_dwMinCount = 0;
	m_CanvasWidth = 0;
	m_CanvasHeight = 0;

	m_SelectStatus = SelectStatus_SelectNone;
	m_SelectBeginX = 0;

	Invalidate();
}

#define AXIS_WIDTH				20
#define	CANVAS_BORDER_WIDTH		1
#define CANVAS_TB_MARGIN		10
#define	ONE_TIME_WIDTH			20
#define	HEIGHT_SCALE_SIZE		500	

#define	CANVAS_BKCOLOR			(RGB(255, 255, 255))
#define	CANVAS_BORDER_COLOR		(RGB(0, 0, 0))
#define	CLIENT_BKCOLOR			(RGB(200, 200, 200))
#define	AXIS_BKCOLOR			(RGB(200, 100, 100))
void CShionChartView::DrawLineChart(std::vector<COUNT_TIME>* p)
{
	if (NULL == p)
	{
		return ;
	}

	m_dwMinCount = p->at(0).dwCount;
	m_dwMaxCount = p->at(0).dwCount;

	for (UINT i=0; i<p->size(); i++)
	{
		if (p->at(i).dwCount < m_dwMinCount)
		{
			m_dwMinCount = p->at(i).dwCount;
		}

		if (p->at(i).dwCount > m_dwMaxCount)
		{
			m_dwMaxCount = p->at(i).dwCount;
		}
	}

	m_pVec = p;

	CClientDC dcClient(m_hWnd);

	DWORD dwHeightScale = 1;

	// ��������canvas�Ŀ�ȡ��߶�
	m_CanvasWidth = m_pVec->size() * ONE_TIME_WIDTH;
	m_CanvasHeight = m_dwMaxCount - m_dwMinCount;
	if (0 == m_CanvasHeight)
	{
		m_CanvasHeight = m_CanvasHeight + 1;
		dwHeightScale = HEIGHT_SCALE_SIZE / m_CanvasHeight;
	}
	else
	{
		dwHeightScale = HEIGHT_SCALE_SIZE / m_CanvasHeight;
	}

	m_CanvasHeight = m_CanvasHeight * dwHeightScale;
	m_CanvasHeight = m_CanvasHeight + 2 * CANVAS_TB_MARGIN;

	// ��rcCanvas��size��������DC��bitmap
	CBitmap CanvasBitMap;
	CanvasBitMap.CreateCompatibleBitmap(dcClient, m_CanvasWidth, m_CanvasHeight);
	// ��Canvas��bitmapѡ���ڴ�DC
	m_CanvasDC.SelectBitmap(CanvasBitMap);

	// ���ڴ�DC����canvas
	m_CanvasDC.FillSolidRect(0, 0, m_CanvasWidth, m_CanvasHeight, CANVAS_BKCOLOR);			//����

	// ����Line
	if (m_pVec->size() > 0)
	{	
		Graphics graphics(m_CanvasDC.m_hDC);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

		Point ptPrev;
		Point ptCur;
		ptPrev.X = ONE_TIME_WIDTH;
		ptPrev.Y = (m_CanvasHeight - (m_pVec->at(0).dwCount - m_dwMinCount) * dwHeightScale) - CANVAS_TB_MARGIN;
		for (UINT i=1; i<m_pVec->size(); i++)
		{
			ptCur.X = (i+1) * ONE_TIME_WIDTH;
			ptCur.Y = (m_CanvasHeight - (m_pVec->at(i).dwCount - m_dwMinCount) * dwHeightScale) - CANVAS_TB_MARGIN;
			graphics.DrawLine(m_pChartLinePen, ptPrev, ptCur);
			ptPrev = ptCur;
		}
	}

	SetScrollSize(m_CanvasWidth, 1, false);
	Invalidate();
}

void CShionChartView::GetSelectedIndexRange(DWORD& dwBegin, DWORD& dwEnd)
{
	dwBegin = 0;
	dwEnd = 0;
	
	if (m_SelectStatus == SelectStatus_Selected)
	{
		dwBegin = (m_SelectBeginX - m_CanvasRect.left ) / ONE_TIME_WIDTH;
		dwEnd = (m_SelectEndX - m_CanvasRect.left ) / ONE_TIME_WIDTH;
	}

	return ;
}

void CShionChartView::DoPaint(CDCHandle dc)
{
	// ��ȡclient����dc
	CClientDC dcClient(m_hWnd);
	// ���λ��ƣ���ʼ���ڴ�DC�͸��ֻ�ˢ
	if (TRUE == m_ImageDC.IsNull() || 
		TRUE == m_CanvasDC.IsNull())
	{
		m_ImageDC.CreateCompatibleDC(dcClient);
		m_CanvasDC.CreateCompatibleDC(dcClient);
	}

	// ��ȡclient����rect
	CRect rcClient;
	GetClientRect(rcClient);

	CRect rcImage = rcClient;
	rcImage.right = AXIS_WIDTH + CANVAS_BORDER_WIDTH + m_CanvasWidth + CANVAS_BORDER_WIDTH;

	// ����Y������rect
	CRect rcCountAxis = rcImage;
	rcCountAxis.top = AXIS_WIDTH;
	rcCountAxis.right = AXIS_WIDTH;

	// ����X������rect
	CRect rcTimeAxis = rcImage;
	rcTimeAxis.left = AXIS_WIDTH;
	rcTimeAxis.bottom = AXIS_WIDTH;

	// ���컭���߿�rect
	CRect rcCanvasBorder;
	rcCanvasBorder = rcImage;
	rcCanvasBorder.top = AXIS_WIDTH;
	rcCanvasBorder.left = AXIS_WIDTH;

	// ������ƻ�������rect
	m_CanvasRect = rcImage;
	m_CanvasRect.top = AXIS_WIDTH + CANVAS_BORDER_WIDTH;
	m_CanvasRect.left = AXIS_WIDTH + CANVAS_BORDER_WIDTH;
	m_CanvasRect.right = m_CanvasRect.right - CANVAS_BORDER_WIDTH;
	m_CanvasRect.bottom = m_CanvasRect.bottom - CANVAS_BORDER_WIDTH;

	// ��ȡClient�����DC�ļ���bitmap
	CBitmap BitMap;
	BitMap.CreateCompatibleBitmap(dcClient, rcImage.Width(), rcImage.Height());
	m_ImageDC.SelectBitmap(BitMap);

	// ���ڴ�Image DC��ͼ
	m_ImageDC.FillSolidRect(rcTimeAxis, AXIS_BKCOLOR);
	m_ImageDC.FillSolidRect(rcCountAxis, AXIS_BKCOLOR);
	m_ImageDC.FillSolidRect(rcCanvasBorder, CANVAS_BORDER_COLOR);
	m_ImageDC.StretchBlt(m_CanvasRect.left, m_CanvasRect.top, m_CanvasRect.Width(), m_CanvasRect.Height(), 
		m_CanvasDC, 0, 0, m_CanvasWidth, m_CanvasHeight, SRCCOPY);

	Graphics graphics(m_ImageDC.m_hDC);

	if (m_SelectStatus == SelectStatus_Selecting)
	{
		// ��ǰ����ѡ���У������Ȱ���ʼ��Ĳο��߻���Image
		// ����Ϊ�˽�����Ѿ�ѡ���״̬���ٴε�����ת����ѡ����״̬ʱ����ʼ�ο��߱����������
		graphics.DrawLine(m_pSelectLinePen, m_SelectBeginX, rcClient.top, m_SelectBeginX, rcClient.bottom);
	}
	
	if (m_SelectStatus == SelectStatus_Selected)
	{
		// ��ǰ����Ӧѡ���״̬��ÿ�ζ�����RECT����ʼ�ο��ߡ������ο���
		graphics.DrawLine(m_pSelectLinePen, m_SelectBeginX, rcClient.top, m_SelectBeginX, rcClient.bottom);
		graphics.FillRectangle(m_pSelectRectBrush, m_SelectBeginX, m_CanvasRect.top, m_SelectEndX - m_SelectBeginX, m_CanvasRect.Height());
		graphics.DrawLine(m_pSelectLinePen, m_SelectEndX, rcClient.top, m_SelectEndX, rcClient.bottom);
	}

	// ��������ɵ��ڴ�DC��ͼ����ʾ����������
	dc.FillSolidRect(rcClient, CLIENT_BKCOLOR);
	dc.BitBlt(0, 0, rcImage.Width(), rcImage.Height(), m_ImageDC, 0, 0, SRCCOPY);

	if (m_SelectStatus == SelectStatus_SelectNone)
	{
		POINT ptCursor;
		POINT ptHittest;
		POINT ptOffset;

		GetScrollOffset(ptOffset);
		GetCursorPos(&ptCursor);
		ScreenToClient(&ptCursor);

		ptHittest.x = ptCursor.x + ptOffset.x;
		ptHittest.y = ptCursor.y;

		if (TRUE == m_CanvasRect.PtInRect(ptHittest))
		{
			Graphics graphics(dc.m_hDC);
			graphics.DrawLine(m_pSelectLinePen, ptCursor.x, rcClient.top, ptCursor.x, rcClient.bottom);
		}
	}
}

LRESULT CShionChartView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT CShionChartView::OnLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT ptHittest;			// ����ó����ڲ����Ƿ�����Canvas��POINT
	POINT ptCursor;				// ��������Client��POINT
	POINT ptOffset;				// ��ͼ������ƫ��

	GetScrollOffset(ptOffset);
	ptCursor.x = GET_X_LPARAM(lParam);
	ptCursor.y = GET_Y_LPARAM(lParam);
	ptHittest.x = ptCursor.x + ptOffset.x;
	ptHittest.y = ptCursor.y;

	if (FALSE == m_CanvasRect.PtInRect(ptHittest))
	{
		return 0;
	}

	switch (m_SelectStatus)
	{
	case SelectStatus_SelectNone:			// ����δ����״̬
	case SelectStatus_Selected:				// ���������ѡ�е�״̬����ʧЧclient����
		Invalidate();
		m_SelectBeginX = ptCursor.x;
		m_SelectStatus = SelectStatus_Selecting;
		break;

	case SelectStatus_Selecting:			// �������ѡ����״̬��������굯�����
		OnLButtonUp(WM_LBUTTONUP, wParam, lParam, bHandled);
		break;

	default:
		break;
	}

	return 0;
}

/************************************************************************/
/* �����Client�����ƶ�ʱ�Ļ��Ʋ���
 * 
 * �˺����в���˫�������                                                                     */
/************************************************************************/
LRESULT CShionChartView::OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	static LPARAM sLastParam = 0;		// ��ס�ϴε�����
	if (sLastParam == lParam)			
	{
		// ���û���ƶ���ֱ�ӷ���
		return 0;
	}
	sLastParam = lParam;

	static LONG sLastXInCanvas = 0;

	POINT ptHittest;					// ����ó����ڲ����Ƿ�����Canvas��POINT
	POINT ptCursor;						// ��������Client��POINT
	POINT ptOffset;						// ��ͼ������ƫ��

	GetScrollOffset(ptOffset);			// ��ȡ��ǰ��ͼ�������������ƫ����

	ptCursor.x = GET_X_LPARAM(lParam);
	ptCursor.y = GET_Y_LPARAM(lParam);
	ptHittest.x = ptCursor.x + ptOffset.x;
	ptHittest.y = ptCursor.y;

	CRect rcClient;
	GetClientRect(rcClient);
	
	CClientDC dcClient(m_hWnd);	

	// �ڴ�DC
	CMemoryDC dcMemory(dcClient, rcClient);
	dcMemory.FillSolidRect(rcClient, CLIENT_BKCOLOR);
	dcMemory.BitBlt(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), m_ImageDC, ptOffset.x, 0, SRCCOPY);

	INT nLeft = 0;
	INT nRight = 0;
	LONG lLineX = 0;

	if (TRUE == m_CanvasRect.PtInRect(ptHittest))
	{
		nLeft = min(m_SelectBeginX, ptCursor.x);
		nRight = max(m_SelectBeginX, ptCursor.x);
		lLineX = ptCursor.x;

		sLastXInCanvas = ptCursor.x;

		// ���㵱ǰ���λ�õ�ͼ�������������
		if (NULL != m_pVec)
		{	
			UINT nIndex = (ptHittest.x - m_CanvasRect.left ) / ONE_TIME_WIDTH;

			CString strToolTip;
			strToolTip.Format(_T("Time:%d\r\nCount:%d"), m_pVec->at(nIndex).dwTime, m_pVec->at(nIndex).dwCount);

			m_ToolTip.UpdateTipText(strToolTip.GetString(), m_hWnd);
			m_ToolTip.Activate(TRUE);
			m_ToolTip.TrackPosition(ptCursor.x, ptCursor.y);
		}
	}
	else
	{
		nLeft = min(m_SelectBeginX, sLastXInCanvas);
		nRight = max(m_SelectBeginX, sLastXInCanvas);
		lLineX = sLastXInCanvas;

		m_ToolTip.Activate(FALSE);
	}
	
	Graphics graphics(dcMemory.m_hDC);
	
	switch (m_SelectStatus)
	{
	case SelectStatus_SelectNone:
		{
			// ���Ƶ�ǰ���ѡ����
			graphics.DrawLine(m_pSelectLinePen, lLineX, rcClient.top, lLineX, rcClient.bottom);
		}
		break;
	case SelectStatus_Selecting:
		{
			// ������ʼ���ѡ����
			graphics.DrawLine(m_pSelectLinePen, m_SelectBeginX, rcClient.top, m_SelectBeginX, rcClient.bottom);
			// ������ʼ�㵽��ǰ���Rect
			graphics.FillRectangle(m_pSelectRectBrush, nLeft, m_CanvasRect.top, nRight - nLeft, m_CanvasRect.Height());
			// ���Ƶ�ǰ��ѡ����
			graphics.DrawLine(m_pSelectLinePen, lLineX, rcClient.top, lLineX, rcClient.bottom);
		}
		break;
	case SelectStatus_Selected:
		break;
	default:
		break;
	}

	dcClient.BitBlt(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), dcMemory, ptOffset.x, 0, SRCCOPY);

	return 0;
}

LRESULT CShionChartView::OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	POINT ptHittest;			// ����ó����ڲ����Ƿ�����Canvas��POINT
	POINT ptCursor;				// ��������Client��POINT
	POINT ptOffset;				// ��ͼ������ƫ��

	if (m_SelectStatus == SelectStatus_Selecting)
	{	
		// ֮ǰ����ѡ�������
		GetScrollOffset(ptOffset);
		ptCursor.x = GET_X_LPARAM(lParam);
		ptCursor.y = GET_Y_LPARAM(lParam);
		ptHittest.x = ptCursor.x + ptOffset.x;
		ptHittest.y = ptCursor.y;

		if (TRUE == m_CanvasRect.PtInRect(ptHittest))
		{
			if (m_SelectBeginX != ptCursor.x)
			{
				// �ɿ����ʱλ�úͰ������ʱλ�ò�ͬ
				m_SelectEndX = max(m_SelectBeginX, ptCursor.x);
				m_SelectBeginX = min(m_SelectBeginX, ptCursor.x);

				m_SelectBeginX = m_SelectBeginX + ptOffset.x;
				m_SelectEndX = m_SelectEndX + ptOffset.x;

				m_SelectStatus = SelectStatus_Selected;	
			}
			else
			{
				// �ɿ����ʱ�Ͱ������ʱλ����ͬ
				m_SelectStatus = SelectStatus_SelectNone;
			}

			Invalidate();
		}
	}

	return 0;
}

BOOL CShionChartView::IsSelected()
{
	return m_SelectStatus == SelectStatus_Selected;
}

LRESULT CShionChartView::OnRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	NMHDR nmHeader;
	nmHeader.code = NM_RCLICK;
	nmHeader.hwndFrom = m_hWnd;
	nmHeader.idFrom = 0;
	SendMessage(GetParent(), WM_NOTIFY, 0, (LPARAM)&nmHeader);
	return 0;
}
