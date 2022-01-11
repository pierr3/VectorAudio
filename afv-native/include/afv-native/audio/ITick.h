//
//  ITick.h
//  afv_native
//
//  Created by Mike Evans on 11/20/20.
//

#ifndef ITick_h
#define ITick_h

namespace afv_native {
    namespace audio {
        /** ISampleStorage is a recorded buffer that can have one or more
         * StorageCursorSources attached to it.
         */
        class ITick{
        public:
            virtual void tick() = 0;
        };
    }
}




#endif /* ITick_h */
