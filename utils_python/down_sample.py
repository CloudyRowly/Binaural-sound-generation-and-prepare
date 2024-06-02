import scipy.io.wavfile as wav
import scipy.signal as signal

def downsample_wav(input_file, output_file, target_sample_rate):
    # Read the input WAV file
    sample_rate, data = wav.read(input_file)

    # Calculate the resampling ratio
    resampling_ratio = target_sample_rate / sample_rate

    # Downsample the audio data
    downsampled_data = signal.resample(data, int(len(data) * resampling_ratio))

    # Write the downsampled data to the output WAV file
    wav.write(output_file, target_sample_rate, downsampled_data.astype(data.dtype))

# Usage example
absolute_path = 'D:\\Learning\\y2s2\\engn4213\\assignment2\\audio sample\\util\\gunshot.wav'
abs_output_path = 'D:\\Learning\\y2s2\\engn4213\\assignment2\\audio sample\\util\\gunshot_downscaled.wav'
target_sample_rate = 8000

downsample_wav(absolute_path, abs_output_path, target_sample_rate)