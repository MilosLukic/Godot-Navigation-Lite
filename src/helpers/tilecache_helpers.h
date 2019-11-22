#ifndef TILECACHE_HELPERS_H
#define TILECACHE_HELPERS_H
#include <DetourTileCache.h>
#include <DetourTileCacheBuilder.h>
#include <Recast.h>
#include <fastlz.h>
#include "helpers.h"

namespace godot {

	struct FastLZCompressor : public dtTileCacheCompressor {
		virtual int maxCompressedSize(const int bufferSize) {
			return (int)(bufferSize * 1.05f);
		}

		virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
			unsigned char* compressed,
			const int /*maxCompressedSize*/,
			int* compressedSize) {
			*compressedSize =
				fastlz_compress((const void*)buffer, bufferSize, compressed);
			return DT_SUCCESS;
		}

		virtual dtStatus decompress(const unsigned char* compressed,
			const int compressedSize, unsigned char* buffer,
			const int maxBufferSize, int* bufferSize) {
			*bufferSize =
				fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
			return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
		}
	};
	struct LinearAllocator : public dtTileCacheAlloc {
		unsigned char* buffer;
		size_t capacity;
		size_t top;
		size_t high;

		LinearAllocator(const size_t cap) :
			buffer(0),
			capacity(0),
			top(0),
			high(0) {
			resize(cap);
		}

		~LinearAllocator() { dtFree(buffer); }

		void resize(const size_t cap) {
			if (buffer)
				dtFree(buffer);
			buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
			capacity = cap;
		}

		virtual void reset() {
			high = MAX(high, top);
			top = 0;
		}

		virtual void* alloc(const size_t size) {
			if (!buffer)
				return 0;
			if (top + size > capacity)
				return 0;
			unsigned char* mem = &buffer[top];
			top += size;
			return mem;
		}

		virtual void free(void* /*ptr*/) {
			// Empty
		}
	};

}

#endif