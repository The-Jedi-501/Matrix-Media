#This header section is strictly for the Media Control aspect of fetching the information ie song name, artist, album 
import asyncio #The main piece of code that uses this is the "await" reason being since were dealign with WinRT where essentlly were pausing until windows itself responds thinnk this way we can kinda prep all the code so rather than printing it on line after line and waitign for that infromation itslef were essnetlly prepping then sending it all in one go 
import time #This is for the time.sleep(5) at the end essentally a wait --- THIS ESSENTALLY FREEZES/BLOCKS THE ENIRE PROGAM BY 5 SECONDS WHEN WE REACH END 
from io import BytesIO #io is Python's built-in input/output library, and BytesIO specifically lets you treat raw bytes sitting in memory as if they were an open file — which is what let Image.open() read the thumbnail data without ever saving it to disk first.

from winsdk.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager
)# This is the translation from python and windows oeprating system that gives us media contolsso the 
#first part of winsdk..... its just saying were using winsdk whcih is the python piece but i need you to look into the windows.media.control being a very specific part of windows
#Second part: Global..... is the directory in which we are gettiugn the media from and then the "as MediaManager" think just getting a new varible 

#Headers for the visual side 
from PIL import Image #sorta a library header main idea is to take the image adn put into PILLOW 

from winsdk.windows.storage.streams import Buffer, InputStreamOptions 
#Translation: whilst this is all visual an important part to realize is that were now dealing with WinRT which is Windows Runtime which means it has its own data types, it has an Asycchronous Design whcih jsut means the resouce heavy tasks dont have any issue when i use this program 
#From says: once again same idea of whilst using winsdk to spcilly look at the windows.storage.streams which is just a way for us to get the visual compoent we know as album covers but in the computer its where its thumbnails but more importatnly were using windows directly to grab those thumbnails 
# Second part import: Buffer - is a WinRT type think how in cpp we might say float      InputStreamOptions - is a specific setting we use later called READ_AHEAD 

async def get_media_info(): #all this says besides it beign a function header the async says "were using awaits" that litterally it 
    sessions = await MediaManager.request_async() #first real "ask windows soemthing since await" which also has the renames as part saying give me the object in this media session  
    current_session = sessions.get_current_session() #no await so since we have the info just putting this info somewhere so when i call on it i dont need to wait i already have a varible for it, DO NO NEED AWAIT EVERYWHERE ONLY WHEN NEED TO TALK TO WINDOWS ITSELF, Note its not just about no wait needs it also happens to be local and instant hence no need for await which does take some time  

    if not current_session: # Safety check --- Later idea when at a better spot maybe a dispaly on matrix to literrally show a image that says no image across full 64, 32
        return None

    info = await current_session.try_get_media_properties_async() # another "await" why because now i need the media itself ie song title, artist, album, thumbnail

    thumbnail_bytes = None #default varibale saying we start with no thumbnail which coudl aslo be album art 
    if info.thumbnail: #then were checking does this thumbnail have any information/ a real picture to work with 
        stream = await info.thumbnail.open_read_async() #another "await" becuase we need the actual art piece to see for reading 
        buffer = Buffer(stream.size) # Emppty WinRT sized to fit exalty the image itself 
        await stream.read_async(buffer, stream.size, InputStreamOptions.READ_AHEAD) #Real transfer of data time - but this is a funky line so now buffer has all the contents that now has the image itself 
        thumbnail_bytes = bytes(buffer) #so now were taking what was soemthign python couldnt use and now turning it into somethign we can in the form of python bytes which takes formate of Image.Open()which pillow needs to work with 

    return { #since we did an await erailer to get all this info itslef whcih was from the "info = await current_session .... " line so all its doing is packaging up so for later  its just a hash map for later on when we print to terminal all the information, please not thumbnail_bytes is the odd duck here because it comes from a differnt part of code not the "info = ... " that the rest use 
        "title": info.title,
        "artist": info.artist,
        "album": info.album_title,
        "thumbnail_bytes": thumbnail_bytes
    }


def build_matrix_image(thumbnail_bytes, filename="current_album.png"):
    img = Image.open(BytesIO(thumbnail_bytes)).convert("RGB")
    img_square = img.resize((32, 32))

    canvas = Image.new("RGB", (64, 32), (0, 0, 0))
    canvas.paste(img_square, (0, 0))

    canvas.save(filename)
    return canvas  # handing back the finished image, not just saving it


print("Spotify monitor started...")
print("The format for what we see is: Title of piece, artist") #maybe for rn could add say a color to show better ie song name blue artist red just for sake of reading in terminal for things that dont naturally look right in termianl think a youtube video 

last_song = None

while True:
    try:
        result = asyncio.run(get_media_info())

        if result:
            current_song = (result["title"], result["artist"])

            if current_song != last_song:
                print(f"Now Playing: {result['title']} by {result['artist']}")
                last_song = current_song

                if result["thumbnail_bytes"]:
                    canvas = build_matrix_image(result["thumbnail_bytes"])
                    canvas.show()  # temporary stand-in for the real matrix --- Note EVERY SINGLE NEW SONG OR MEDIA PLAY IS ANOTHER WINDOW SO LATER ON GET RID OF 

    except Exception as e:
        print(f"ERROR: {type(e).__name__}: {e}")

    time.sleep(5)