package se.svep.mood.companion

import android.util.Log
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import kotlin.concurrent.thread

/**
 * Minimal HTTP listener for the pkjs bridge. Bound to loopback only — nothing
 * is reachable from the network. Routes:
 *   POST /import       — the export batch (config + registrations)
 *   GET  /pending      — queued phone-side config changes for the watch
 *   POST /pending-ack  — pkjs confirmed which changes landed
 *
 * Deliberately hand-rolled (no Ktor/NanoHTTPD); it only needs to parse one
 * shape of request from one well-known client.
 */
class ImportServer(
    private val port: Int = 9099,
    /** Returns the JSON response body, or null for 404. */
    private val route: (method: String, path: String, body: String) -> String?,
) {
    @Volatile private var serverSocket: ServerSocket? = null

    fun start() {
        if (serverSocket != null) return
        val socket = ServerSocket(port, 4, InetAddress.getLoopbackAddress())
        serverSocket = socket
        thread(isDaemon = true, name = "mood-import-server") {
            Log.i(TAG, "listening on 127.0.0.1:$port")
            while (serverSocket != null) {
                try {
                    socket.accept().use(::handle)
                } catch (e: Exception) {
                    if (serverSocket != null) Log.w(TAG, "accept failed", e)
                }
            }
        }
    }

    fun stop() {
        val socket = serverSocket
        serverSocket = null
        socket?.close()
    }

    private fun handle(client: Socket) {
        val reader = BufferedReader(InputStreamReader(client.getInputStream(), Charsets.UTF_8))
        val requestLine = reader.readLine() ?: return
        val parts = requestLine.split(" ")
        val method = parts.getOrElse(0) { "" }
        val path = parts.getOrElse(1) { "" }.substringBefore('?')

        var contentLength = 0
        while (true) {
            val line = reader.readLine() ?: break
            if (line.isEmpty()) break
            val (name, value) = line.split(":", limit = 2).let {
                it[0].trim().lowercase() to it.getOrElse(1) { "" }.trim()
            }
            if (name == "content-length") contentLength = value.toIntOrNull() ?: 0
        }

        var body = ""
        if (contentLength > 0) {
            val chars = CharArray(contentLength)
            var read = 0
            while (read < contentLength) {
                val n = reader.read(chars, read, contentLength - read)
                if (n < 0) break
                read += n
            }
            body = String(chars, 0, read)
        }

        val response = try {
            route(method, path, body)
        } catch (e: Exception) {
            Log.e(TAG, "handler failed for $method $path", e)
            "{\"status\":\"error\"}"
        }

        val status = if (response != null) "200 OK" else "404 Not Found"
        val payload = response ?: "{\"status\":\"unknown endpoint\"}"
        client.getOutputStream().write(
            ("HTTP/1.1 $status\r\n" +
                "Content-Type: application/json\r\n" +
                "Content-Length: ${payload.toByteArray(Charsets.UTF_8).size}\r\n" +
                "Connection: close\r\n\r\n" +
                payload).toByteArray(Charsets.UTF_8)
        )
        client.getOutputStream().flush()
    }

    private companion object {
        const val TAG = "MoodImportServer"
    }
}
