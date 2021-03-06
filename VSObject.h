#pragma once


struct VSO_Vertex
{
    D3DXVECTOR3 pos; // Position
	D3DXVECTOR2 tex; // Texturecoords
};


struct CURVE_Vertex
{
    D3DXVECTOR2 pos;	// Position and Blurr
	D3DXVECTOR4 col[2];		// Color: left, right
	D3DXVECTOR2	nb;			// previous vertex and next vertex
};


struct COLORPOINT
{
	D3DXVECTOR3 col;
	int			off;
};

struct BLURRPOINT
{
	float		blurr;
	int			off;
};


struct CURVE
{
	int			pNum;
	D3DXVECTOR2 *p;
	int			clNum;
	COLORPOINT	*cl;
	int			crNum;
	COLORPOINT  *cr;
	int			bNum;
	BLURRPOINT *b;
};

class VSObject
{
public:
	CComPtr<ID3DX10Mesh> m_pMeshDiff;
	CComPtr<ID3DX10Mesh> m_pMeshCurves;

	ID3D10Texture2D *m_diffuseTexture[2];     // two textures used interleavedly for diffusion
	ID3D10Texture2D *m_distDirTexture;    // two textures used interleavedly for diffusion (blurr texture)
	ID3D10Texture2D *m_pDepthStencil;         // for z culling
	ID3D10Texture2D *m_otherTexture;		// texture that keeps the color on the other side of a curve

public:
	ID3D10ShaderResourceView *m_diffuseTextureRV[2];
	ID3D10RenderTargetView   *m_diffuseTextureTV[2];
	ID3D10ShaderResourceView *m_distDirTextureRV;
	ID3D10RenderTargetView   *m_distDirTextureTV;
	ID3D10DepthStencilView   *m_pDepthStencilView;
	ID3D10ShaderResourceView *m_otherTextureRV;
	ID3D10RenderTargetView   *m_otherTextureTV;

	ID3D10EffectTechnique *m_pDrawVectorsTechnique;
	ID3D10EffectTechnique *m_pDiffuseTechnique;
	ID3D10EffectTechnique *m_pLineAntiAliasTechnique;
	ID3D10EffectTechnique *m_pDisplayImage;

	ID3D10EffectShaderResourceVariable* m_pInTex[3];
	ID3D10EffectShaderResourceVariable* m_pDiffTex;
	ID3D10EffectScalarVariable* m_pDiffX;
	ID3D10EffectScalarVariable* m_pDiffY;
	ID3D10EffectScalarVariable* g_pScale;
	ID3D10EffectVectorVariable* g_pPan;
	ID3D10EffectScalarVariable* m_pPolySize;
	ID3D10EffectScalarVariable* m_pGreyValue;

	D3D10_VIEWPORT m_vp;

	float m_sizeX;
	float m_sizeY;
	int diffSteps;
	float m_scale;
	D3DXVECTOR2 m_pan;
	float m_polySize;
	int	m_cNum;
	int m_cSegNum;
	CURVE *m_curve;
	float m_fWidth;
	float m_fHeight;
	int diffTex;
	int diff2Tex;

	static D3D10_INPUT_ELEMENT_DESC InputElements[];
	static D3D10_INPUT_ELEMENT_DESC InputCurveElements[];
	static int InputElementCount;
	static int InputCurveElementCount;
	ID3D10Buffer *m_pVertexBuffer;
	ID3D10InputLayout* m_pVertexLayout;

	VSObject(ID3D10Device *pd3dDevice);
	~VSObject(void);

	ID3D10ShaderResourceView *GetDisplayTextureRV(void) { return m_diffuseTextureRV[diffTex]; }

	BOOL CreateVertexBuffer(ID3D10Device* pd3dDevice);
	BOOL CreateIndexBuffer(ID3D10Device *pd3dDevice);
	
	void SetupTextures(ID3D10Device *pd3dDevice, ID3D10Effect* g_pEffect10, int sizeX, int sizeY);
	//mode = 0: all colors of the file are loaded
	//mode = 1: all colors of the file are white
	//mode = 2: all colors of the file are black
	void ReadVectorFile( char *s, int mode);
	void ConstructCurves(ID3D10Device *pd3dDevice);
};
