package se.svep.mood.companion

import android.util.Log
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import kotlin.concurrent.thread

/**
 * Minimal HTTP listener for the pkjs export delivery (POST /import).
 *
 * The Pebble phone app's JS runtime POSTs the full registration batch to
 * http://127.0.0.1:9099/import (see src/pkjs/index.js in the repo root).
 * Bound to loopback only — nothing is reachable from the network.
 *
 * Deliberately hand-rolled (no Ktor/NanoHTTPD) while this is a spike; it only
 * needs to parse one shape of request from one well-known client.
 */
class ImportServer(
    private val port: Int = 9099,
    private val onImport: (body: String) -> Unit,
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

        var contentLength = 0
        while (true) {
            val line = reader.readLine() ?: break
            if (line.isEmpty()) break
            val (name, value) = line.split(":", limit = 2).let {
                it[0].trim().lowercase() to it.getOrElse(1) { "" }.trim()
            }
            if (name == "content-length") contentLength = value.toIntOrNull() ?: 0
        }

        val ok = requestLine.startsWith("POST") && requestLine.contains("/import")
        if (ok && contentLength > 0) {
            val body = CharArray(contentLength)
            var read = 0
            while (read < contentLength) {
                val n = reader.read(body, read, contentLength - read)
                if (n < 0) break
                read += n
            }
            onImport(String(body, 0, read))
        }

        val status = if (ok) "200 OK" else "404 Not Found"
        val payload = if (ok) "{\"status\":\"ok\"}" else "{\"status\":\"unknown endpoint\"}"
        client.getOutputStream().write(
            ("HTTP/1.1 $status\r\n" +
                "Content-Type: application/json\r\n" +
                "Content-Length: ${payload.length}\r\n" +
                "Connection: close\r\n\r\n" +
                payload).toByteArray(Charsets.UTF_8)
        )
        client.getOutputStream().flush()
    }

    private companion object {
        const val TAG = "MoodImportServer"
    }
}
