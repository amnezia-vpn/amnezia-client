
#if !defined(AFX_QR_ENCODE_H__AC886DF7_C0AE_4C9F_AC7A_FCDA8CB1DD37__INCLUDED_)
#define AFX_QR_ENCODE_H__AC886DF7_C0AE_4C9F_AC7A_FCDA8CB1DD37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////

//
#define QR_LEVEL_L	0
#define QR_LEVEL_M	1
#define QR_LEVEL_Q	2
#define QR_LEVEL_H	3

//
#define QR_MODE_NUMERAL		0
#define QR_MODE_ALPHABET	1
#define QR_MODE_8BIT		2
#define QR_MODE_KANJI		3

// 
#define QR_VRESION_S	0 
#define QR_VRESION_M	1 
#define QR_VRESION_L	2 

#define MAX_ALLCODEWORD	 3706 
#define MAX_DATACODEWORD 2956 
#define MAX_CODEBLOCK	  153 
#define MAX_MODULESIZE	  177 

#define QR_MARGIN	0


/////////////////////////////////////////////////////////////////////////////
typedef struct tagRS_BLOCKINFO
{
	int ncRSBlock;		
	int ncAllCodeWord;	
	int ncDataCodeWord;	

} RS_BLOCKINFO, *LPRS_BLOCKINFO;


/////////////////////////////////////////////////////////////////////////////

typedef struct tagQR_VERSIONINFO
{
	int nVersionNo;	   
	int ncAllCodeWord; 

	int ncDataCodeWord[4];	

	int ncAlignPoint;	
	int nAlignPoint[6];	

	RS_BLOCKINFO RS_BlockInfo1[4]; 
	RS_BLOCKINFO RS_BlockInfo2[4]; 

} QR_VERSIONINFO, *LPQR_VERSIONINFO;


/////////////////////////////////////////////////////////////////////////////

class CQR_Encode
{
public:
	CQR_Encode();
	~CQR_Encode();

public:
	int m_nLevel;		
	int m_nVersion;		
	bool m_bAutoExtent;	
	int m_nMaskingNo;	

public:
	int m_nSymbleSize;
	unsigned char m_byModuleData[MAX_MODULESIZE][MAX_MODULESIZE]; // [x][y]

private:
	int m_ncDataCodeWordBit; 
	unsigned char m_byDataCodeWord[MAX_DATACODEWORD]; 

	int m_ncDataBlock;
	unsigned char m_byBlockMode[MAX_DATACODEWORD];
	int m_nBlockLength[MAX_DATACODEWORD];

	int m_ncAllCodeWord; 
	unsigned char m_byAllCodeWord[MAX_ALLCODEWORD]; 
	unsigned char m_byRSWork[MAX_CODEBLOCK]; 

public:
	bool EncodeData(int nLevel, int nVersion, bool bAutoExtent, int nMaskingNo, char* lpsSource, int ncSource = 0);

private:
	int GetEncodeVersion(int nVersion, char* lpsSource, int ncLength);
	bool EncodeSourceData(char* lpsSource, int ncLength, int nVerGroup);

	int GetBitLength(unsigned char nMode, int ncData, int nVerGroup);

	int SetBitStream(int nIndex, unsigned short wData, int ncData);

	bool IsNumeralData(unsigned char c);
	bool IsAlphabetData(unsigned char c);
	bool IsKanjiData(unsigned char c1, unsigned char c2);

	unsigned char AlphabetToBinaly(unsigned char c);
	unsigned short KanjiToBinaly(unsigned short wc);

	void GetRSCodeWord(unsigned char * lpbyRSWork, int ncDataCodeWord, int ncRSCodeWord);

private:
	void FormatModule();

	void SetFunctionModule();
	void SetFinderPattern(int x, int y);
	void SetAlignmentPattern(int x, int y);
	void SetVersionPattern();
	void SetCodeWordPattern();
	void SetMaskingPattern(int nPatternNo);
	void SetFormatInfoPattern(int nPatternNo);
	int CountPenalty();
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_QR_ENCODE_H__AC886DF7_C0AE_4C9F_AC7A_FCDA8CB1DD37__INCLUDED_)
