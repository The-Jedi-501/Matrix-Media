# import asyncio
# from io import BytesIO
# from PIL import Image
# from winsdk.windows.media.control import \
#     GlobalSystemMediaTransportControlsSessionManager as MediaManager

# async def get_media_info():
#     sessions = await MediaManager.request_async()
#     current_session = sessions.get_current_session()

#     if current_session:
#         info = await current_session.try_get_media_properties_async()

#         thumbnail_bytes = None
#         if info.thumbnail:
#             stream_ref = info.thumbnail
#             stream = await stream_ref.open_read_async()
#             size = stream.size
#             buffer = bytearray(size)
#             await stream.read_async(buffer, size, 0)
#             thumbnail_bytes = bytes(buffer)

#         return {
#             "title": info.title,
#             "artist": info.artist,
#             "album": info.album_title,
#             "thumbnail_bytes": thumbnail_bytes
#         }
#     return None

# def save_thumbnail(thumbnail_bytes, filename="current_album.png"):
#     img = Image.open(BytesIO(thumbnail_bytes)).convert("RGB")
#     img_resized = img.resize((64, 32), Image.LANCZOS)
#     img_resized.save(filename)

# result = asyncio.run(get_media_info())

# if result:
#     print(f"Now playing: {result['title']} — {result['artist']} ({result['album']})")
#     if result["thumbnail_bytes"]:
#         save_thumbnail(result["thumbnail_bytes"])
#         print("Saved resized preview to current_album.png")
# else:
#     print("Nothing playing right now.")import asyncio
import asyncio
from winsdk.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager
)


from io import BytesIO
from PIL import Image
#from winsdk.windows.media.control import \ GlobalSystemMediaTransportControlsSessionManager as MediaManager

# async def get_media_info():
#     print("Requesting session manager...", flush=True)
#     sessions = await MediaManager.request_async()
#     print("Got session manager.", flush=True)

#     current_session = sessions.get_current_session()
#     print(f"Current session: {current_session}", flush=True)

#     if current_session:
#         info = await current_session.try_get_media_properties_async()
#         print(f"Got properties. Title: {info.title}", flush=True)

#         thumbnail_bytes = None
#         if info.thumbnail:
#             stream_ref = info.thumbnail
#             stream = await stream_ref.open_read_async()
#             size = stream.size
#             buffer = bytearray(size)
#             await stream.read_async(buffer, size, 0)
#             thumbnail_bytes = bytes(buffer)
#             print("Thumbnail read successfully.", flush=True)

#         return {
#             "title": info.title,
#             "artist": info.artist,
#             "album": info.album_title,
#             "thumbnail_bytes": thumbnail_bytes
#         }

async def get_media_info():
    print("Requesting session manager...", flush=True)

    sessions = await MediaManager.request_async()
    print("Got session manager.", flush=True)

    current_session = sessions.get_current_session()
    print(f"Current session: {current_session}", flush=True)

    if not current_session:
        return None

    info = await current_session.try_get_media_properties_async()

    print("Successfully retrieved media properties.", flush=True)

    return {
        "title": info.title,
        "artist": info.artist,
        "album": info.album_title
    }
    
    
    return None

def save_thumbnail(thumbnail_bytes, filename="current_album.png"):
    img = Image.open(BytesIO(thumbnail_bytes)).convert("RGB")
    img_resized = img.resize((64, 32), Image.LANCZOS)
    img_resized.save(filename)

# print("Script starting...", flush=True)

# try:
#     result = asyncio.run(get_media_info())

#     if result:
#         print(f"Now playing: {result['title']} by {result['artist']} (album: {result['album']})", flush=True)
#         if result["thumbnail_bytes"]:
#             save_thumbnail(result["thumbnail_bytes"])
#             print("Saved resized preview to current_album.png", flush=True)
#     else:
#         print("Nothing playing right now.", flush=True)

# except Exception as e:
#     print(f"ERROR: {type(e).__name__}: {e}", flush=True)

# print("Script finished.", flush=True)


print("Script starting...", flush=True)

try:
    result = asyncio.run(get_media_info())

    if result:
        print("\nNow Playing")
        print("----------------------")
        print(f"Title : {result['title']}")
        print(f"Artist: {result['artist']}")
        print(f"Album : {result['album']}")
    else:
        print("Nothing is currently playing.")

except Exception as e:
    print(f"ERROR: {type(e).__name__}: {e}")

print("Script finished.")