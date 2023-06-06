import uvicorn
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates

import datetime

app = FastAPI(title="RFID Project", version="Arquiteturas para Sistemas Embutidos")

templates = Jinja2Templates(directory="templates")


@app.get("/", response_class=HTMLResponse)
async def dashboard(request: Request):
    with open("LOGFILE", "r") as f:
        logs = f.readlines()
    
    return templates.TemplateResponse("index.html", {"request": request, "logs": logs[::-1]})


@app.post("/add_access")
async def add_access(card_number: str):
    with open("ACCESS", "a") as f:
        f.write(card_number)

    return {"message": "Access added successfully for card number " + card_number}


@app.post("/remove_access")
async def remove_access(card_number: str):
    with open("ACCESS", "r") as f:
        serial_numbers = f.readlines()

    with open("ACCESS", "w") as f:
        for sn in serial_numbers:
            if sn.replace("\n", "") != card_number:
                print(sn)
                f.write(sn)

    return {"message": "Access removed successfully for card number " + card_number}


@app.post("/check_access")
async def check_access(data: dict):

    print(data)

    with open("ACCESS", "r") as f:
        serial_numbers = f.readlines()

    if data["sn"] in serial_numbers:
        result = 1
    else:
        result = 0

    if result:
        access = "granted"
    else:
        access = "denied"

    with open("LOGFILE", "a") as f:
        f.write(f"[{datetime.datetime.now()}] : Access {access} (card number {data['sn']})\n")
    
    return result

if __name__ == "__main__":
    uvicorn.run("main:app", host="192.168.43.241", port=80, reload=True)
