/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef vaapidecoder_mpeg2_h
#define vaapidecoder_mpeg2_h

#include "codecparsers/mpeg2_parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va.h"
#include <list>
#include <tr1/functional>
#include <vector>

namespace YamiMediaCodec {

enum Mpeg2PictureStructureType {
    kTopField = 1,
    kBottomField,
    kFramePicture,
};

enum {
    MPEG2_MAX_REFERENCE_PICTURES = 2,
};

class VaapiDecPictureMpeg2;

class VaapiDecoderMPEG2 : public VaapiDecoderBase {
public:
    typedef SharedPtr<VaapiDecPictureMpeg2> PicturePtr;
    VaapiDecoderMPEG2();
    virtual ~VaapiDecoderMPEG2();
    virtual Decode_Status start(VideoConfigBuffer*);
    virtual Decode_Status reset(VideoConfigBuffer*);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer*);

private:
    // mpeg2 DPB class
    class DPB {
        typedef std::tr1::function<Decode_Status(const PicturePtr&)>
            OutputCallback;

    public:
        DPB(OutputCallback callback)
            : m_numberSurfaces(MPEG2_MAX_REFERENCE_PICTURES)
            , m_outputPicture(callback)
        {
        }

        void flush();
        Decode_Status insertPicture(const PicturePtr& picture);
        Decode_Status insertPictureToReferences(const PicturePtr& picture);

        Decode_Status getReferencePictures(const PicturePtr& current_picture,
                                           PicturePtr& previous_picture,
                                           PicturePtr& next_picture);
        Decode_Status callOutputPicture(const PicturePtr& picture)
        {
            return m_outputPicture(picture);
        }

        Decode_Status outputPreviousPictures(const PicturePtr& picture,
                                             bool empty = false);

    private:
        // set to minimum number of surfaces required to operate
        uint32_t m_numberSurfaces;
        std::list<PicturePtr> m_referencePictures;
        OutputCallback m_outputPicture;
    };

    // mpeg2 DPB class

    typedef SharedPtr<YamiParser::MPEG2::Parser> ParserPtr;
    typedef SharedPtr<YamiParser::MPEG2::StreamHeader> StreamHdrPtr;

    friend class FactoryTest<IVideoDecoder, VaapiDecoderMPEG2>;
    friend class VaapiDecoderMPEG2Test;

    void fillSliceParams(VASliceParameterBufferMPEG2* slice_param,
                         const YamiParser::MPEG2::Slice* slice);
    void fillPictureParams(VAPictureParameterBufferMPEG2* param,
                           const PicturePtr& picture);

    Decode_Status fillConfigBuffer();
    Decode_Status
    convertToVAProfile(const YamiParser::MPEG2::ProfileType& profile);
    Decode_Status checkLevel(const YamiParser::MPEG2::LevelType& level);
    bool isSliceCode(YamiParser::MPEG2::StartCodeType next_code);

    Decode_Status processConfigBuffer();
    Decode_Status processDecodeBuffer();

    Decode_Status preDecode(StreamHdrPtr shdr);
    Decode_Status processSlice();
    Decode_Status decodeGOP();
    Decode_Status assignSurface();
    Decode_Status assignPicture();
    Decode_Status createPicture();
    Decode_Status loadIQMatrix();
    Decode_Status decodePicture();
    Decode_Status outputPicture(const PicturePtr& picture);
    Decode_Status findReusePicture(std::list<PicturePtr>& list, bool& reuse);

    ParserPtr m_parser;
    StreamHdrPtr m_stream;

    const YamiParser::MPEG2::SeqHeader* m_sequenceHeader;
    const YamiParser::MPEG2::SeqExtension* m_sequenceExtension;
    const YamiParser::MPEG2::GOPHeader* m_GOPHeader; //check what's the use here
    const YamiParser::MPEG2::PictureHeader* m_pictureHeader;
    const YamiParser::MPEG2::PictureCodingExtension* m_pictureCodingExtension;
    DPB m_DPB;
    VASliceParameterBufferMPEG2* mpeg2SliceParams;

    bool m_VAStart;
    bool m_isParsingSlices;
    PicturePtr m_currentPicture;
    YamiParser::MPEG2::StartCodeType m_previousStartCode;
    YamiParser::MPEG2::StartCodeType m_nextStartCode;
    VAProfile m_VAProfile;
    uint64_t m_currentPTS;

    std::list<PicturePtr> m_topFieldPictures;
    std::list<PicturePtr> m_bottomFieldPictures;

    static const bool s_registered; // VaapiDecoderFactory registration result
};
} // namespace YamiMediaCodec

#endif