#Purpose of the code:


# To Run the program its just:      py At2.py       thats it nothing else 


#Things that are needed to run this program:
# - DONOT USE WSL USE POWERSHELL 
# - must install        py -m pip install winsdk pillow     also make sure python and pip are up to date via    py -m pip
#                       - windsk    this is the python talkign to windows media api which get the current song, artist, cover 
#                       - pillow    this is the image library handles the opening and resiizing of album art - this is suppose to be the big thing to make it fit onto matrix 

# Errors I ran into that i could fix later:
# - because i dont have spotify premuim cannt use web api to view it that way instead going off of the windows media control itself 

# This project was built as a learning exercise. I used official documentation, tutorials, open-source examples, and AI tools to learn unfamiliar libraries and APIs. As I learned, I modified, extended, and debugged the implementation to better understand how the system works. The goal of this repository is to document my learning process and the engineering decisions I made along the way.


import asyncio
import time

from winsdk.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager
)

from PIL import Image #sorta a library header main idea is to take the image adn put into PILLOW 
from io import BytesIO #is taking raw bytes in memory to tehn turn them into a file this is also a big part of async 

async def get_media_info():
    print("Requesting session manager...", flush=True)

    sessions = await MediaManager.request_async()

    current_session = sessions.get_current_session()

    if not current_session:
        return None

    info = await current_session.try_get_media_properties_async()

    return {
        "title": info.title,
        "artist": info.artist,
        "album": info.album_title
    }


print("Spotify monitor started...")

last_song = None

while True:
    try:
        result = asyncio.run(get_media_info())

        if result:
            current_song = (
                result["title"],
                result["artist"]
            )

            if current_song != last_song:
                print("\nNow Playing")
                print("----------------------")
                print(f"Title : {result['title']}")
                print(f"Artist: {result['artist']}")
                print(f"Album : {result['album']}")

                last_song = current_song

    except Exception as e:
        print(f"ERROR: {type(e).__name__}: {e}")

    time.sleep(5)