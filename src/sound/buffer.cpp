#include <cstring>
#include <stdexcept>
#include <string>

#include "buffer.h"
#include "constframe.h"
#include "shared/iresampler.h"
#include "shared/log.h"
#include "shared/server.h"

template <typename T>
Buffer<T>::Buffer()
    : _channels(0)
    , _frames(0)
    , _samples()
{
}

template <typename T>
template <typename S>
Buffer<T>::Buffer(ConstFrame<S> sbeg, ConstFrame<S> send)
    : Buffer<T>()
{
    assign(sbeg, send);
}

template <typename T>
Buffer<T>::Buffer(unsigned channels, unsigned frames)
    : _channels(channels)
    , _frames(frames)
    , _samples(channels * frames, static_cast<T>(0))
{
}

template <typename T>
template <typename S>
void Buffer<T>::assign(ConstFrame<S> sbeg, ConstFrame<S> send)
{
    unsigned channels = sbeg.channels();
    reallocate(channels, send - sbeg);
    copy(sbeg, send);
}

template <typename T>
template <typename S>
void Buffer<T>::assign(const S* ptr, unsigned channels, unsigned frames)
{
    auto sbeg = ConstFrame<S>(channels, reinterpret_cast<const Sample<S>*>(ptr));
    assign(sbeg, sbeg + frames);
}

template <typename T>
Frame<T> Buffer<T>::begin()
{
    return Frame<T>(_channels, _samples.data());
}

template <typename T>
template <typename S>
Frame<T> Buffer<T>::copy(ConstFrame<S> sbeg, ConstFrame<S> send)
{
    if (sbeg.channels() != send.channels()) {
        std::string message = "Begin and end frame channels count differs";
        LOG(ERROR, message
                << ". Begin channels: " << sbeg.channels()
                << ", end channels: " << send.channels());
        throw std::out_of_range(message);
    }

    int count = send - sbeg;

    if (count < 0) {
        std::string message = "End frame is before begin frame on copy";
        LOG(ERROR, message);
        throw std::out_of_range(message);
    }

    if (_frames < unsigned(count)) {
        std::string message = "Buffer overflow";
        LOG(ERROR, message
                << ". Buffer frames: " << _frames << " vs " << count);
        throw std::out_of_range(message);
    }

    if (this->type() == sbeg.type() && _channels == sbeg.channels()) {
        std::memcpy(this->data(), sbeg.data(), count * _channels * sizeof(T));
    } else {
        Frame<T> dit = begin();

        for (ConstFrame<S> sit = sbeg; sit != send; ++sit, ++dit) {
            dit = sit;
        }
    }

    return begin() + count;
}

template <typename T>
ConstFrame<T> Buffer<T>::cbegin() const
{
    return ConstFrame<T>(_channels, &*_samples.cbegin());
}

template <typename T>
ConstFrame<T> Buffer<T>::cend() const
{
    return ConstFrame<T>(_channels, &*_samples.cend());
}

template <typename T>
unsigned Buffer<T>::channels() const
{
    return _channels;
}

template <typename T>
Frame<T> Buffer<T>::end()
{
    return Frame<T>(_channels, _samples.data());
}

template <typename T>
unsigned Buffer<T>::frames() const
{
    return _frames;
}

template <typename T>
bool Buffer<T>::isEmpty() const
{
    return _frames == 0;
}

template <typename T>
Sample<T>* Buffer<T>::data()
{
    return _samples.data();
}

template <typename T>
const Sample<T>* Buffer<T>::data() const
{
    return _samples.data();
}

template <typename T>
void Buffer<T>::nativeInt24(void* dest)
{
    typedef std::uint8_t Int24Parts[3];
    union Int32Parts {
        std::int32_t i32;
        std::uint8_t i8[4];
    };

    const Int32Parts endiannessCheck = { 0x01020304 };
    const bool isBigEndian = 0x01 == endiannessCheck.i8[3];

    auto int24dest = reinterpret_cast<Int24Parts*>(dest);
    auto ptr = _samples.data();
    unsigned offset = unsigned(!isBigEndian);

    for (unsigned i = 0; i < size(); i++) {
        Sample<Sound::Int32> i32 = ptr[i];
        Int32Parts s = { i32 };
        Int24Parts& d = int24dest[i];
        d[0] = s.i8[0 + offset];
        d[1] = s.i8[1 + offset];
        d[2] = s.i8[2 + offset];
    }
}

template <typename T>
void Buffer<T>::reallocate(unsigned channels, unsigned frames)
{
    if (_channels != channels || _frames != frames) {
        if (_channels * _frames < channels * frames) {
            _samples.clear();
        }

        _samples.resize(channels * frames);
        _channels = channels;
        _frames = frames;
    }
}

template <typename T>
void Buffer<T>::resize(unsigned frames)
{
    if (_frames != frames) {
        _samples.resize(_channels * frames);
        _frames = frames;
    }
}

template <typename T>
Buffer<T> Buffer<T>::resample(unsigned destRate, unsigned sourceRate,
    Sound::Quality quality)
{
    Buffer<T> result;
    result.resample(cbegin(), cend(), destRate, sourceRate, quality);
    return result;
}

template <typename T>
void Buffer<T>::resample(ConstFrame<T> sbeg, ConstFrame<T> send,
    unsigned destRate, unsigned sourceRate,
    Sound::Quality quality)
{
    double ratio = double(destRate) / double(sourceRate);
    unsigned sframes = unsigned(send - sbeg);
    unsigned channels = sbeg.channels();
    unsigned dframes = unsigned(double(sframes) * ratio);
    auto resampler = PLUGIN_FACTORY(IResampler);

    if (this->type() == Sound::TypeFloat32) {
        Buffer<Sound::Float32> dtemp(channels, dframes);
        Buffer<Sound::Float32> stemp(sbeg, send);
        resampler->simple(dtemp.begin().ptr(), dframes,
            stemp.begin().ptr(), sframes,
            channels, ratio, quality);
        assign(dtemp.cbegin(), dtemp.cend());
    }
}

template <typename T>
void Buffer<T>::silence()
{
    silence(begin(), end());
}

template <typename T>
void Buffer<T>::silence(Frame<T> dbeg, Frame<T> dend)
{
    auto myBegin = begin();
    auto myEnd = end();

    if (myBegin - dbeg < 0 || dbeg - myEnd < 0
        || myBegin - dend < 0 || dend - myEnd < 0
        || dend - dbeg < 0) {
        std::string message = "Frame out of buffer bounds";
        LOG(ERROR, message);
        throw std::out_of_range(message);
    }

    std::memset(dbeg.ptr(), 0, dend.ptr() - dbeg.ptr());
}

template <typename T>
unsigned Buffer<T>::size() const
{
    return _samples.size();
}

#define BUFFER_COPY(__ttype, __mtype) \
    template Frame<__ttype> Buffer<__ttype>::copy(ConstFrame<__mtype> sbeg, ConstFrame<__mtype> send);

SOUND_INSTANTIATE_TEMPLATE_METHOD(BUFFER_COPY);
SOUND_INSTANTIATE(Buffer);