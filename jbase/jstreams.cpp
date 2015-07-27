
// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jwarningpragmas.hpp"

#include "jstreams.hpp"

#include <algorithm>
#include <string.h>
#include <errno.h>

using namespace jjm;
using namespace std;


namespace
{
    int const g_defaultBufferSize = 64 * 1024; //MAGIC NUMBER
}

jjm::BufferedInputStream::BufferedInputStream(
                InputStream * inputStream, std::size_t bufferSize) 
    : m_inputStream(NULL), 
    m_isBad(false),
    m_isEof(false), 
    m_gcount(0), 
    m_allocBegin(NULL),
    m_allocEnd(NULL),
    m_dataBegin(NULL),
    m_dataEnd(NULL)
{
    resetInputStream(inputStream, bufferSize);
} 

jjm::BufferedInputStream::~BufferedInputStream()
{
    delete[] m_allocBegin;
}

void jjm::BufferedInputStream::resetInputStream(
                InputStream * inputStream, std::size_t bufferSize)
{
    //get the default size
    if (bufferSize == 0)
        bufferSize = g_defaultBufferSize;

    //free it if we want a new size, or if the new InputStream is null
    if (inputStream == NULL || (m_allocBegin != NULL && (m_allocBegin + bufferSize != m_allocEnd)))
    {   delete[] m_allocBegin;
        m_allocBegin = NULL;
        m_allocEnd = NULL;
        m_dataBegin = NULL;
        m_dataEnd = NULL;
    }

    if (inputStream && m_allocBegin == NULL)
    {   m_allocBegin = new unsigned char[bufferSize]; 
        m_allocEnd = m_allocBegin + bufferSize; 
        m_dataBegin = m_allocBegin;
        m_dataEnd = m_allocBegin; 
    }

    m_inputStream = inputStream;
    m_isBad = false;
    m_isEof = false;
    m_gcount = 0; 
}

jjm::BufferedOutputStream::BufferedOutputStream(
                OutputStream * outputStream, std::size_t bufferSize) 
    : m_outputStream(NULL), 
    m_isBad(false),
    m_gcount(0), 
    m_allocBegin(NULL),
    m_allocEnd(NULL),
    m_dataBegin(NULL),
    m_dataEnd(NULL)
{
    resetOutputStream(outputStream, bufferSize);
} 

jjm::BufferedOutputStream::~BufferedOutputStream()
{
    if (m_outputStream)
        flush();
    delete[] m_allocBegin;
}

void jjm::BufferedOutputStream::resetOutputStream(
                OutputStream * outputStream, std::size_t bufferSize)
{
    //get the default buffer size
    if (bufferSize == 0)
        bufferSize = g_defaultBufferSize;

    //free it if we want a new size, or if the new InputStream is null
    if (outputStream == NULL || (m_allocBegin != NULL && (m_allocBegin + bufferSize != m_allocEnd)))
    {   delete[] m_allocBegin;
        m_allocBegin = NULL;
        m_allocEnd = NULL;
        m_dataBegin = NULL;
        m_dataEnd = NULL;
    }

    if (outputStream && m_allocBegin == NULL)
    {   m_allocBegin = new unsigned char[bufferSize];
        m_allocEnd = m_allocBegin + bufferSize; 
        m_dataBegin = m_allocBegin;
        m_dataEnd = m_allocBegin; 
    }

    m_outputStream = outputStream;
    m_isBad = false; 
    m_gcount = 0; 
}

void jjm::BufferedInputStream::read2(void * argBlock_, std::size_t argBlockSize)
{
    unsigned char * argBlock = static_cast<unsigned char*>(argBlock_);

    for (;;)
    {
        if (argBlockSize == 0)
            return; 

        //First use whatever is in our internal buffer to satisfy the request. 
        {
            ssize_t toCopy = std::min<ssize_t>(argBlockSize, m_dataEnd - m_dataBegin);
            if (toCopy > 0)
            {   memcpy(argBlock, m_dataBegin, toCopy);
                m_dataBegin += toCopy; 
                argBlock += toCopy;
                argBlockSize -= toCopy;
                m_gcount += toCopy; 

                if (argBlockSize == 0)
                    return; 
            }
        }

        //At this point, our internal buffer must be empty. 
        m_dataBegin = m_allocBegin;
        m_dataEnd = m_allocBegin;

        //As a heuristic, if the request size is larger than our internal
        //buffer capacity, then skip internal buffering and go straight to the 
        //underlying InputStream. 
        if (argBlockSize + m_allocBegin <= m_allocEnd)
        {
            //refill our internal buffer, 
            ssize_t const x = m_inputStream->read(m_dataBegin, m_allocEnd - m_dataBegin);
            if (x >= 0)
            {   //success
                m_dataEnd = m_dataBegin + x; 
                continue;
            }
            if (x == -1)
            {   m_isBad = true; 
                m_isEof = true; 
                return; 
            }
            m_isBad = true; 
            return; 
        }else
        {
            ssize_t const x = m_inputStream->read(argBlock, argBlockSize);
            if (x >= 0) 
            {   //success
                argBlock += x; 
                argBlockSize -= x;
                m_gcount += x; 
                continue;
            }
            if (x == -1)
            {   //eof
                m_isBad = true; 
                m_isEof = true; 
                return; 
            }
            //some error
            m_isBad = true; 
            return; 
        }
    }
}

void jjm::BufferedOutputStream::write2(void const * argBlock_, std::size_t argBlockSize)
{
    unsigned char const * argBlock = static_cast<unsigned char const*>(argBlock_);

    for (;;)
    {
        if (argBlockSize == 0)
            return; 

        //As a heuristic, if the request size is larger than our internal
        //buffer capacity, then skip internal buffering and go straight to the 
        //underlying OutputStream. 

        if (argBlockSize + m_allocBegin <= m_allocEnd)
        {
            //put what can fit into our internal buffer, and flush our internal buffer
            ssize_t const toCopy = std::min<ssize_t>(m_allocEnd - m_dataEnd, argBlockSize); 
            memcpy(m_dataEnd, argBlock, toCopy);
            m_dataEnd += toCopy;
            argBlock += toCopy;
            argBlockSize -= toCopy;
            m_gcount += toCopy; 
            if ( ! flushInternalBuffer())
                return; 

            //write the remaining amount to our now-empty internal buffer
            memcpy(m_dataEnd, argBlock, argBlockSize);
            m_dataEnd += argBlockSize; 
            m_gcount += argBlockSize;
            return; 
        }else
        {
            //Flush the internal buffer (it needs to be written before newer data), 
            //and then go to the OutputStream directly. 
            if ( ! flushInternalBuffer())
                return; 
            ssize_t x = m_outputStream->write(argBlock, argBlockSize);
            if (x >= 0)
            {   //success
                argBlock += x;
                argBlockSize -= x; 
                m_gcount += x; 
                continue; 
            }
            m_isBad = true; 
            return; 
        }
    }
}

jjm::BufferedOutputStream & jjm::BufferedOutputStream::flush()
{
    if ( ! flushInternalBuffer())
        return *this; 
    if (0 != m_outputStream->flush())
        m_isBad = true; 
    return *this; 
}

jjm::BufferedOutputStream & jjm::BufferedOutputStream::flushInternalBuffer()
{
    if (!*this)
        return *this;
    for ( ; m_dataBegin < m_dataEnd; )
    {   ssize_t x = m_outputStream->write(m_dataBegin, m_dataEnd - m_dataBegin);
        if (x < 0)
        {   m_isBad = true; 
            return *this;
        }
        m_dataBegin += x;
    }
    m_dataBegin = m_allocBegin;
    m_dataEnd = m_allocBegin;
    return *this; 
}

void jjm::BufferedOutputStream::resetState()
{
    m_isBad = false; 
    m_gcount = 0;
}

void jjm::BufferedInputStream::resetState()
{
    m_isBad = false; 
    m_isEof = false; 
    m_gcount = 0;
}

int64_t jjm::BufferedInputStream::seek(int64_t off, int whence)
{
    if (m_isBad)
        return -1; 

    //The apparent position of the buffered input stream is different than
    //the position of the underlying stream. 
    //That that into account in the offset value when seeking from current 
    //position. 
    if (SEEK_CUR == whence)
    {   ssize_t internalBufferSize = m_dataEnd - m_dataBegin;
        off -= internalBufferSize; 
    }

    int64_t result = m_inputStream->seek(off, whence);

    //Position of underlying stream has been moved. 
    //Clear our internal buffer
    m_dataBegin = m_allocBegin;
    m_dataEnd = m_allocBegin;

    return result; 
}

int64_t jjm::BufferedOutputStream::seek(int64_t off, int whence)
{
    if (m_isBad)
        return -1; 

    //The apparent position of the buffered input stream is different than
    //the position of the underlying stream. 
    //That that into account by flushing our internal buffer. 
    flush();
    if (m_isBad)
        return -1;

    int64_t result = m_outputStream->seek(off, whence);
    return result; 
}
