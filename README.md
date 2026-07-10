= ffmpeg-circular-buffer

Multithreaded circular buffer loading for ffmpeg video/audio streaming application.`read_packet` function and `CyclicFragmentBuffer` can be passed to ffmpeg `avio_alloc_context`. Most of the test are paired to bugs found while using in real online streaming player <https://github.com/sqrtminustwo/VideoPlayer>.
