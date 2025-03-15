package com.example.emotiondetector

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.ColorMatrix
import android.graphics.ColorMatrixColorFilter
import android.graphics.Paint
import android.os.Bundle
import android.widget.Toast
import androidx.compose.foundation.border
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.example.emotiondetector.ui.theme.EmotionDetectorTheme
import androidx.activity.compose.rememberLauncherForActivityResult
import org.tensorflow.lite.Interpreter
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.MappedByteBuffer
import java.nio.channels.FileChannel
import java.io.FileInputStream

class MainActivity : ComponentActivity() {
    private var capturedImageBitmap by mutableStateOf<Bitmap?>(null)
    private var detectedEmotionText by mutableStateOf<String?>(null)
    private lateinit var tflite: Interpreter

    // Define emotion labels
    private val emotionLabels = listOf("Angry", "Disgust", "Fear", "Happy", "Sad", "Surprise", "Neutral")

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            openCamera()
        } else {
            Toast.makeText(this, "Camera permission is required", Toast.LENGTH_SHORT).show()
        }
    }

    private val takePictureLauncher = registerForActivityResult(
        ActivityResultContracts.TakePicturePreview()
    ) { bitmap ->
        if (bitmap != null) {
            capturedImageBitmap = bitmap
            detectedEmotionText = null
            Toast.makeText(this, "Image captured successfully!", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        try {
            tflite = Interpreter(loadModelFile())
        } catch (e: Exception) {
            Toast.makeText(this, "Failed to load model: ${e.message}", Toast.LENGTH_LONG).show()
        }

        setContent {
            EmotionDetectorTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    EmotionDetectorApp(
                        modifier = Modifier.padding(innerPadding),
                        onCameraClick = { checkCameraPermissionAndOpen() },
                        capturedImage = capturedImageBitmap,
                        onAnalyzeClick = { analyzeEmotion() },
                        detectedEmotion = detectedEmotionText
                    )
                }
            }
        }
    }

    private fun checkCameraPermissionAndOpen() {
        when {
            ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED -> {
                openCamera()
            }
            else -> {
                requestPermissionLauncher.launch(Manifest.permission.CAMERA)
            }
        }
    }

    private fun openCamera() {
        takePictureLauncher.launch(null)
    }

    private fun loadModelFile(): MappedByteBuffer {
        val fileDescriptor = assets.openFd("emotion_model.tflite")
        val inputStream = FileInputStream(fileDescriptor.fileDescriptor)
        val fileChannel = inputStream.channel
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, fileDescriptor.startOffset, fileDescriptor.declaredLength)
    }

    private fun analyzeEmotion() {
        capturedImageBitmap?.let { bitmap ->
            try {
                val resizedBitmap = Bitmap.createScaledBitmap(bitmap, 48, 48, true)

                val grayscaleBitmap = convertToGrayscale(resizedBitmap)

                val inputBuffer = convertBitmapToByteBuffer(grayscaleBitmap)

                val outputBuffer = Array(1) { FloatArray(7) }

                tflite.run(inputBuffer, outputBuffer)

                val maxIndex = outputBuffer[0].indices.maxByOrNull { outputBuffer[0][it] } ?: -1

                val emotionLabel = if (maxIndex >= 0 && maxIndex < emotionLabels.size) {
                    emotionLabels[maxIndex]
                } else {
                    "Unknown"
                }

                val confidence = (outputBuffer[0][maxIndex] * 100).toInt()

                detectedEmotionText = "$emotionLabel ($confidence%)"

            } catch (e: Exception) {
                detectedEmotionText = "Error: ${e.message}"
                Toast.makeText(this, "Analysis failed: ${e.message}", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun convertToGrayscale(bitmap: Bitmap): Bitmap {
        val grayscaleBitmap = Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888)
        val canvas = android.graphics.Canvas(grayscaleBitmap)
        val paint = Paint()
        val colorMatrix = ColorMatrix()
        colorMatrix.setSaturation(0f)
        paint.colorFilter = ColorMatrixColorFilter(colorMatrix)
        canvas.drawBitmap(bitmap, 0f, 0f, paint)
        return grayscaleBitmap
    }

    private fun convertBitmapToByteBuffer(bitmap: Bitmap): ByteBuffer {
        val buffer = ByteBuffer.allocateDirect(48 * 48 * 4)
        buffer.order(ByteOrder.nativeOrder())

        for (y in 0 until 48) {
            for (x in 0 until 48) {
                val pixel = bitmap.getPixel(x, y)

                val r = (pixel shr 16 and 0xFF)
                val g = (pixel shr 8 and 0xFF)
                val b = (pixel and 0xFF)
                val gray = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f

                buffer.putFloat(gray)
            }
        }

        buffer.rewind()
        return buffer
    }
}

@Composable
fun EmotionDetectorApp(
    modifier: Modifier = Modifier,
    onCameraClick: () -> Unit,
    capturedImage: Bitmap?,
    onAnalyzeClick: () -> Unit,
    detectedEmotion: String?
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Text(
            text = "Emotion Detector",
            style = MaterialTheme.typography.headlineMedium,
            modifier = Modifier.fillMaxWidth()
        )

        Box(
            modifier = Modifier
                .size(300.dp)
                .border(2.dp, androidx.compose.ui.graphics.Color.Gray, androidx.compose.foundation.shape.RoundedCornerShape(8.dp)),
            contentAlignment = Alignment.Center
        ) {
            if (capturedImage != null) {
                Image(
                    bitmap = capturedImage.asImageBitmap(),
                    contentDescription = "Captured Image",
                    modifier = Modifier.fillMaxSize()
                )
            } else {
                Text("No image captured yet")
            }
        }

        detectedEmotion?.let {
            Text(
                text = "Detected: $it",
                style = MaterialTheme.typography.titleLarge,
                color = MaterialTheme.colorScheme.primary
            )
        }

        Button(
            onClick = onCameraClick,
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp)
        ) {
            Text("Take Photo")
        }

        if (capturedImage != null) {
            Button(
                onClick = onAnalyzeClick,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp)
            ) {
                Text("Analyze Emotion")
            }
        }
    }
}