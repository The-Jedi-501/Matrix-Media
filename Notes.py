#Purpose of the code: To be able to learn and understand some new concepts before the Second year at uni

#What I wanted to acheive in this process was to be able to Create an LED matrix that woudl be able to take images in the form of album covers or whats refered to in this code is thumbnails and display them both thru the py code itself and also via the matrix 

#About my comments: I have ALOT of comments as im using this as my own personal notes for not only how various sections of code work as I am using mulitple libraries and using many different example of code and working with and adjusting to fit into my project. MAKE A SECOND VERSION OF THIS CODE  one that has all the comment ie this one and other is just this section do this thats it so it more clean rather than comment vomit so NotedVersion.py and Clean.py do at very end tho 

# To Run the program its just:      py At2.py (or whatever the name of file is i keep adding more)       thats it nothing else 


#Things that are needed to run this program:
# - DO NOT USE WSL USE POWERSHELL for both installing of following but also when runnign the program itself 
# - must install        py -m pip install winsdk pillow     also make sure python and pip are up to date via    py -m pip
#                       - windsk    this is the python talkign to windows media api which get the current song, artist, cover 
#                       - pillow    this is the image library handles the opening and resiizing of album art - this is suppose to be the big thing to make it fit onto matrix 
#                       - pyserial  this is to allow python script to read from and write to a serial port --- essentlly how were going to get the python code to talk to the esp32 cpp code to the desired com port 

# Errors I ran into that i could fix later:
# - because i dont have spotify premuim cannt use web api to view it that way instead going off of the windows media control itself 

# This project was built as a learning exercise. I used official documentation, tutorials, open-source examples, and AI tools to learn unfamiliar libraries and APIs. As I learned, I modified, extended, and debugged the implementation to better understand how the system works. The goal of this repository is to document my learning process and the engineering decisions I made along the way.

#Things learnt (can be wicked dumb):
# - Shitf + Tab is a thing in vs code to unindent code





# --- HEADERS --- theres alot to talk about since each library has ALOT to go over 

#This header section is strictly for the Media Control aspect of fetching the information ie song name, artist, album 
import asyncio #The main piece of code that uses this is the "await" reason being since were dealign with WinRT where essentlly were pausing until windows itself responds thinnk this way we can kinda prep all the code so rather than printing it on line after line and waitign for that infromation itslef were essnetlly prepping then sending it all in one go 
import time #This is for the time.sleep(5) at the end essentally a wait --- THIS ESSENTALLY FREEZES/BLOCKS THE ENIRE PROGAM BY 5 SECONDS WHEN WE REACH END 
from io import BytesIO
from turtle import title #io is Python's built-in input/output library, and BytesIO specifically lets you treat raw bytes sitting in memory as if they were an open file — which is what let Image.open() read the thumbnail data without ever saving it to disk first.

from winsdk.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager
)# This is the translation from python and windows oeprating system that gives us media contolsso the 
#first part of winsdk..... its just saying were using winsdk whcih is the python piece but i need you to look into the windows.media.control being a very specific part of windows
#Second part: Global..... is the directory in which we are gettiugn the media from and then the "as MediaManager" think just getting a new varible 


#Headers for the visual side 
from PIL import Image #sorta a library header main idea is to take the image adn put into PILLOW 
from PIL import ImageDraw #This allows for custom drawin of images 

from winsdk.windows.storage.streams import Buffer, InputStreamOptions 
#Translation: whilst this is all visual an important part to realize is that were now dealing with WinRT which is Windows Runtime which means it has its own data types, it has an Asycchronous Design whcih jsut means the resouce heavy tasks dont have any issue when i use this program 
#From says: once again same idea of whilst using winsdk to spcilly look at the windows.storage.streams which is just a way for us to get the visual compoent we know as album covers but in the computer its where its thumbnails but more importatnly were using windows directly to grab those thumbnails 
# Second part import: Buffer - is a WinRT type think how in cpp we might say float      InputStreamOptions - is a specific setting we use later called READ_AHEAD 


# Headers here coorelate to how were communicating via serial port ie "COM3" or in plain english "Using this usp port at this part of my computer"
import serial 

from datetime import datetime, timezone # Need to get real time for the elapsed time 



# --- REAL CODE STARTS HERE --- 

# --- Getting the python to talk to COM3 --- 
try: #try is literally just saying can you try this one thing before you crash the program 
    ser = serial.Serial('COM3', 115200)  # match your ESP32's port and baud rate --- this MUST CORRESPOND TO THE ARDUNIO CODE 
    print("ESP32 connected.")
    time.sleep(3)  # give the ESP32 time to finish its boot/setup sequence --- very important need this here at least in my testing otherwise didnt work 
except Exception as e: #"if it failed branch" think e as an error object 
    ser = None
    print(f"Could not connect to ESP32: {e}")

# --- ASYNC or what essenlly us getting  all the information from media control ---
async def get_media_info(): #all this says besides it beign a function header the async says "were using awaits" that litterally it 
    sessions = await MediaManager.request_async() #first real "ask windows soemthing since await" which also has the renames as part saying give me the object in this media session  
    current_session = sessions.get_current_session() #no await so since we have the info just putting this info somewhere so when i call on it i dont need to wait i already have a varible for it, DO NO NEED AWAIT EVERYWHERE ONLY WHEN NEED TO TALK TO WINDOWS ITSELF, Note its not just about no wait needs it also happens to be local and instant hence no need for await which does take some time  


    if not current_session: # Safety check --- Later idea when at a better spot maybe a dispaly on matrix to literrally show a image that says no image across full 64, 32
        return None   
    
    timeline = current_session.get_timeline_properties()
    #print([attr for attr in dir(timeline) if not attr.startswith('_')])
    #print([attr for attr in dir(current_session) if not attr.startswith('_')]) #Use this to determine what is available to us in the current session object, this is a good way to see what you can use and what you cannot use
    
    playback_info = current_session.get_playback_info()#used to get the playback status of the current session, which can be useful for determining if a song is playing, paused, or stopped. This information can be used to control the display on the LED matrix or to trigger other actions in the program based on the playback state.
    
    #print([attr for attr in dir(playback_info) if not attr.startswith('_')])
    #print(timeline.position, type(timeline.position))
    # print(timeline.end_time, type(timeline.end_time))
    
    print([attr for attr in dir(playback_info) if not attr.startswith('_')])
    print(playback_info.playback_status)
    
    
    
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
        "thumbnail_bytes": thumbnail_bytes,
                #dont need start but to demo 
        "started_at": timeline.start_time,#This section is for trackign the song and how far along it is in the song itself
        "position": timeline.position, 
        "last_updated_position": timeline.last_updated_time,
        "end_time": timeline.end_time, 
        
        "playback_status": playback_info.playback_status, #This is the status of the song itself ie is it playing, paused, or stopped
        
    }

    
# --- Pillow which is us adjusting the given thumbnail itself to fit and move wher i need it to be ---  
def build_matrix_image(thumbnail_bytes, filename="current_album.png"): #funciton uses the thumbnail bytes which is raw image from SMTC and were going to put it into this file "current_album.png"
    img = Image.open(BytesIO(thumbnail_bytes)).convert("RGB") #reading the image and using pillow to convert which im using a 3 value per pixed hence RBG rather than a tuple (4) for RGBA as A=alpha (transpartent)
    img_square = img.resize((32, 32)) # resize the image to 32 x 32 regardless of size

    canvas = Image.new("RGB", (64, 32), (0, 0, 0)) # Im taking the orignal matrix and making it black to start 
    canvas.paste(img_square, (0, 0)) #putting that resized image onto that black canvas at a specific starting point that being left most so 0,0 and the right side would still be black 

    canvas.save(filename) # were saving the finished/desired image to the file 
    return canvas  # handing back the finished image



# --- Logic for how we take that image and actully send it/print/make the art on THE MATRIX --- Still using some of the Pillow ---
def send_image_to_matrix(canvas): # Taking one input that being THE Resized and Adjusted image from function "build_matrix_image"
    
    if ser is None: #Safecty check from earlier verion of code new "try and exception" at top of code which just tells user we cannot connect, None is literally nothing there cant use 0 or False as those have values 
        print("No ESP32 connection — skipping matrix update.")
        return
    
    
    pixel_bytes = bytearray() # Empty container holding raw bytes so 0-255 --- think as a list/array/vector is empty here weere doing the same but with byte but its holding byte values --- bytes happens to have the same range as the colors were doing so when we see 255, 0, 0 think oh bytes so its perfect size i
    for y in range(32): #This loop must be the same order ie Y first then X same as the ardunio code as its reading in this order rather then here where were going thru and sending those values in that order 
        for x in range(64):#was 32 but due to right side for staus needed
            r, g, b = canvas.getpixel((x, y)) # Were saying hey in the canvas what color is there we get the RGB hence (r, g, b) 
            pixel_bytes.extend([r, g, b]) # .extend takes that list adn puts each of the 3 items into the pixel_bytes --- this line does 1024 timer total once per pixel by end of code its done 3072 entires hence 1024x3

    message = b"ART:" + bytes(pixel_bytes) #Alot happens here so: b is saying take "ART:" into bytes and ART is the real image which this name must be the same as what its called in esp code but whats importatnt is the : is the delimiter that says "label is done, data starts now" think of this as the green light to send data after we have filled up the new bytes for new art, the + we need to have both sides of this be same format ie both bytes so stick these two things together one after anohter but this time with bytes and we have already coverd the pixel_bytes 
    ser.write(message) #send teh entire combined sequecne over the wire/us in one go 
    

# --- Pause/Play Icon --- 
def draw_status_icon(canvas, is_playing):
    draw = ImageDraw.Draw(canvas)
    
    # Right-side block is x=32 to x=64, y=0 to y=32 — draw roughly centered in it
    if is_playing:
        # Play triangle: three points forming a right-pointing triangle
        draw.polygon([(42, 8), (42, 24), (56, 16)], fill=(0, 255, 0))
    else:
        # Pause bars: two vertical rectangles
        draw.rectangle([40, 8, 46, 24], fill=(255, 0, 0))
        draw.rectangle([50, 8, 56, 24], fill=(255, 0, 0))
    
    return canvas
    
# --- Sending the song info to the ESP32 via serial port ---
def send_text_to_LCD(title, artist): #taking two inputs for this function which is the title and artist of the song 
    if ser is None: #same as above if we cannot connect to esp32 then just skip this part 
        print("No ESP32 connection — skipping LCD update.")
        return

    message = b"TXT:" + f"{title}|{artist}".encode() + b"\n" #same idea as above but now were taking the title and artist and putting into a string with a | in between to say hey this is the end of title and now artist starts, the .encode() is saying take this string and turn it into bytes so we can send it over the wire, the \n is saying hey were done with this message now you can process it and do what you need to do with it 
    ser.write(message) #send the entire combined sequecne over the wire/us in one go
    
# --- Sending the position via time to the ESP32 via serial port ---
def send_position_to_LCD(position_seconds, end_seconds, is_playing): #taking two inputs for this function which is the position and end_time of the song
    if ser is None: #same as above if we cannot connect to esp32 then just skip this part 
        print("No ESP32 connection — skipping LCD update.")
        return

    message = b"POS:" + f"{position_seconds}|{end_seconds}~{is_playing}".encode() + b"\n" #same idea as above but now were taking the position and end_time and putting into a string with a | in between to say hey this is the end of position and now end_time starts, the .encode() is saying take this string and turn it into bytes so we can send it over the wire, the \n is saying hey were done with this message now you can process it and do what you need to do with it 
    ser.write(message) #send the entire combined sequecne over the wire/us in one go

# --- Starting up --- 
print("Spotify monitor started...")
print("The format for what we see is: Title of piece, artist") #maybe for rn could add say a color to show better ie song name blue artist red just for sake of reading in terminal for things that dont naturally look right in termianl think a youtube video 


# --- Real logic that happens constatnly just telling us that songs whos artist and actully sending the information to the functions and giving the infrormation to matrix via COM3 ---
last_song = None

last_update_time = None #Solves of the tuple  --- Never used 

last_playback_status = None   #Tryign to figure out hwo to get a pause icon going 

while True:
    try: 
        result = asyncio.run(get_media_info()) #actully calling async to get all my infroamtion this starts the intial async func call - hence whe we call those key from th dict/hashmap we can get the title, artist, album, and thumbnail_bytes from the result varible

        if result: #hey if we actully get a result then were good this is mixing a if and try real lock and key stuff 
            
            current_song = (result["title"], result["artist"]) #were just building a varibale so we can compare for whats next and important its a tuple as were pairing title and artist 
            #last_update_seen = (result["started_at"], result["position"], result["end_time"])
            
            elapsed = datetime.now(timezone.utc) - result["last_updated_position"] #this is a new varible that is saying hey how long has it been since the last time of update in this code 1 sec
            
            if result["playback_status"] == 4:
                real_position = result["position"] + elapsed
            else:
                real_position = result["position"]
            
            position_seconds = int(real_position.total_seconds())
            end_seconds = int(result["end_time"].total_seconds())
    
            print(f"Position: {position_seconds} / {end_seconds}") #print the position of the song and how long it is in total
            print(f"Position: {real_position} / {result['end_time']}") #print the position of the song and how long it is in total

            #send_position_to_LCD(real_position, result["end_time"]) #calling the function to send the position to the LCD on the matrix --- wouldnt eher ebe two of this but its an if sayign if playback is 4 or just ==t treu 
            
            if result["playback_status"] == 4:
                status = 1
            else:
                status = 0
            #before we send to lcd i want to have a way to track via a variable so if playing status is 1 
            send_position_to_LCD(position_seconds, end_seconds, status) 
            
            if result["last_updated_position"] != last_update_time:
                last_update_time = result["last_updated_position"] #Note the hash value is a tuple so we can compare the entire thing at once rather than each individual part
    
            if current_song != last_song: #are we on a new song yet then print the update to terminal notice use of hash map and to take the key and return the value and f becasue embded variabele 
                
                pipeline_start = time.time()  # start the clock here
                
                print(f"Now Playing: {result['title']} by {result['artist']}")
                last_song = current_song

                send_text_to_LCD(result["title"], result["artist"]) #calling the function to send the text to the LCD on the matrix
                
                if result["thumbnail_bytes"]: #since new song new art, note if same album diff song would visuall look same but still happens but also says if there is a real thumbnail i can grab 
                    
                    canvas = build_matrix_image(result["thumbnail_bytes"]) # build my canvas by taking the raw image we are getting and send to build to resize and adjust 
                    
                    canvas = draw_status_icon(canvas, result["playback_status"] == 4)
                    
#to see the image in termanal for preview uncomment next line 
                    #canvas.show()  # temporary stand-in for the real matrix --- Note EVERY SINGLE NEW SONG OR MEDIA PLAY IS ANOTHER WINDOW SO LATER ON GET RID OF 
                    
                # TESTING - time in terms of why its takign a long time to get images to display on matrix - keep func remove rest later on - the idea was if the eqution we know is giving us values close or not that being resolution x 10 (10 becasue its the rate uart reads 10 bits per byte or bits per frame) / baud rate = time in my case .53 seconds were getting .57-.58 so not noticable 
                    send_start = time.time()
                    send_image_to_matrix(canvas) # callin a function to say im done with this you are all good 
                    send_end = time.time()
                    print(f"[SPAN 1] send_image_to_matrix took {send_end - send_start:.3f}s")

                pipeline_end = time.time()  # <-- stop the clock here
                print(f"[SPAN 2] full pipeline took {pipeline_end - pipeline_start:.3f}s")
                
            if result["playback_status"] != last_playback_status:   # NEW block — sibling, same level
                last_playback_status = result["playback_status"]
                if result["thumbnail_bytes"]:
                    canvas = build_matrix_image(result["thumbnail_bytes"])
                    canvas = draw_status_icon(canvas, result["playback_status"] == 4)
                    send_image_to_matrix(canvas)

    except Exception as e: #exception is hey we couldnt get the infoarmtion in the form of the media info so print the error to terminal --- if error store into 'e varibale '
        print(f"ERROR: {type(e).__name__}: {e}") # ugly part here "{type(e).__name__}: {e}" says type(e) is saying what type of error is this example of output is <class 'serial.SerialException'>, .__name__ says strip that nonsese for me so a person can read it to now "SerialException" ie short name i can read please, and finally {e} is actual object error so could not open port 'COM3': Access is denied, please note the : is not logic its just a printing thing to seperate coudl literlly be anything pay no attention to it 

    time.sleep(1) # Pasue for 5 SECONDS diff from ardunio its ms reason we do this is so its got some time to wait before it pulls a new image and allows for program to kinda breath otherwise I had run into issues
    
