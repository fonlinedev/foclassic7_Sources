#include "Core.h"

#include "GameOptions.h"
#include "GraphicStructures.h"
#include "Log.h"

#ifdef FO_D3D
#else

Texture::Texture() : Name( NULL ), Id( 0 ), Data( NULL ), Size( 0 ), Width( 0 ), Height( 0 ), Samples( 0.0f )
{
    # ifdef FO_WINDOWS
    PBuffer = NULL;
    # endif
}

Texture::~Texture()
{
    GL( glDeleteTextures( 1, &Id ) );
    SAFEDELA( Data );
}

bool Texture::Update()
{
    GL( glBindTexture( GL_TEXTURE_2D, Id ) );

	#ifdef FO_EMSCRIPTEN
		uint8* rgba = new uint8[Width * Height * 4];

		for( uint i = 0; i < Width * Height; i++ )
		{
			rgba[i * 4 + 0] = Data[i * 4 + 2];
			rgba[i * 4 + 1] = Data[i * 4 + 1];
			rgba[i * 4 + 2] = Data[i * 4 + 0];
			rgba[i * 4 + 3] = Data[i * 4 + 3];
		}

		GL( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, Width, Height,
							 GL_RGBA, GL_UNSIGNED_BYTE, rgba ) );

		delete[] rgba;
	#else
    GL( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, Width, Height,
                         GL_BGRA, GL_UNSIGNED_BYTE, Data ) );
	#endif

    return true;
}

bool Texture::Update( const Rect& r )
{
    GL( glBindTexture( GL_TEXTURE_2D, Id ) );

	#ifdef FO_EMSCRIPTEN
		uint width = r.W();
		uint height = r.H();

		uint8* rgba = new uint8[width * height * 4];

		uint8* src = (uint8*)Data + (r.T * Width + r.L) * 4;

		for( uint y = 0; y < height; y++ )
		{
			for( uint x = 0; x < width; x++ )
			{
				uint src_pos = (y * Width + x) * 4;
				uint dst_pos = (y * width + x) * 4;

				rgba[dst_pos + 0] = src[src_pos + 2];
				rgba[dst_pos + 1] = src[src_pos + 1];
				rgba[dst_pos + 2] = src[src_pos + 0];
				rgba[dst_pos + 3] = src[src_pos + 3];
			}
		}

		GL( glTexSubImage2D( GL_TEXTURE_2D, 0, r.L, r.T, width, height,
							 GL_RGBA, GL_UNSIGNED_BYTE, rgba ) );

		delete[] rgba;
	#else
		GL( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, r.T, Width, r.H(),
							 GL_BGRA, GL_UNSIGNED_BYTE,
							 (uint*)Data + r.T * Width ) );
	#endif

    return true;
}

#endif
