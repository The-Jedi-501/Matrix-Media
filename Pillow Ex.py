from PIL import Image
import os #this librabry just telle me where that picture is stored 

# ----- First Iteration ------ Main goal was to kinda get a feel for what im doing 

#img = Image.open("current_album.png") #The png in question relates to a file path apparrently but no search so can do just name of file as we do here or if the fiel is somewhere else then do "C:/Users/chris/Desktop/some_other_folder/current_album.png" ---- so shorthand if same folder else do the long path 

#print(img.size)   # (width, height) in pixels
#print(img.mode)   # color format, e.g. 'RGB' or 'RGBA' -- could do img.format would say png, jpeg, etc

#print(os.getcwd()) # idk didnt work 

#img.show() # liteally opens the image itslef in the size it was so bc we did the resize later we get the raw image so its somethign liek 302,300 

#img_small = img.resize((64, 32))
#print(img_small.size)  # should now print (32, 32) --- the matrix is 64x32 but since i dont want to stretch it idk tho 

#apparently imporatnt later for matix stuff 
# pixel = img_small.getpixel((0, 0))  # the pixel at x=0, y=0 (top-left corner)
# print(pixel)

#----- Secodn iteraiton rather than just having it all be seperat piece we can make it better logically lets go thru it 

with Image.open("current_album.png") as img: #Beneift here over what we did above is so that we can immedauly chage the size of the same file it shrinks memory usage here 
    img_small = img.resize((64, 64)) #was 64, 32 now 32 
    
img_small.show()

#This is the logic for  getting the color for the each pixel themselves -- 0,0 is top left and increases downwards its weird apparntly 
for y in range(64):        # rows (height)
    for x in range(64):    # columns (width)
        pixel = img_small.getpixel((x, y)) #saying that img we have tell me get hte pixel there and only look at that one 
        print(f"({x}, {y}): {pixel}") #i want you to print intermal where its located as well as teh color 

#Main idea with this portion of code that realtes to the real overall project is to essentially have a blank canvas is blank matrix of which i will then i will essnlly chosose wher i want to start the drawring of the art 
# canvas = Image.new("RGB", (64, 32), (0, 0, 0)) #So this tells us to set the matrix to all blank via turning all the led to black 
# canvas.paste(img_square, (0, 0))  # flush against left edge, flush against top --- Then we are saying to paste this starting at 0 0 note img_square in this case woudl jsut be img_small diff varibale casue its a code ex so the number its going off of is from the img.resize () value 

# canvas.show()        