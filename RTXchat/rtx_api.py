from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
import time
import csv

# ----------------def functions-----------------------
def extract_data(input_filename, data_name):
    data_list = []
    with open(input_filename, mode='r', newline='', encoding='utf-8') as file:
        csv_reader = csv.DictReader(file)
        for row in csv_reader:
            data_list.append(row[data_name])
    return data_list

def auto_chat(driver, prompts):
    for prompt in prompts:
        # Locate the input field for the chat message (replace 'input_selector' with the actual selector)
        input_field = driver.find_element("css selector", "input_selector")
        # Enter the chat message into the input field
        input_field.send_keys(prompt)
        # Press ENTER to submit the chat message
        input_field.send_keys(Keys.RETURN)
        # Wait for the response (adjust time as needed)
        time.sleep(5)


# ----------------def functions-----------------------
# Input file (output file varies from Q to Q)
input_file = 'source/question_answer_v2_20240221.csv'


# Extract prompts from the CSV file
prompts = extract_data(input_file, 'Question')  # Replace 'prompt' with the actual column name

# Set up Chrome options
chrome_options = Options()

# Instantiate the Chrome driver
webDriver = webdriver.Chrome(service=Service('chromedriver-win64\chromedriver.exe'))

# Open the ChatGPT URL
webDriver.get('https://chat.openai.com/c/ba4524bd-38a8-4264-b4be-5f4a2bc1f1c6')

# Wait for the page to load
time.sleep(9999)

# Perform auto chat with the extracted prompts
auto_chat(webDriver, prompts)

# Keep the window open for a while to see the results
time.sleep(9999)

# Close the browser
webDriver.close()