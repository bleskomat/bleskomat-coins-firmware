#include "screen/tft.h"

namespace {

	TFT_eSPI tft = TFT_eSPI();
	const uint16_t bgColor = TFT_BLACK;
	uint16_t textColor = TFT_WHITE;
	int16_t center_x;
	int16_t center_y;
	int16_t margin = 4;

	typedef std::vector<GFXfont> FontList;

	FontList monospaceFonts = {
		// Ordered from largest (top) to smallest (bottom).
		Courier_Prime_Code28pt8b,
		Courier_Prime_Code26pt8b,
		Courier_Prime_Code24pt8b,
		Courier_Prime_Code22pt8b,
		Courier_Prime_Code20pt8b,
		Courier_Prime_Code18pt8b,
		Courier_Prime_Code16pt8b,
		Courier_Prime_Code14pt8b,
		Courier_Prime_Code12pt8b,
		Courier_Prime_Code10pt8b,
		Courier_Prime_Code9pt8b,
		Courier_Prime_Code8pt8b
	};

	FontList brandFonts = {
		// Ordered from largest (top) to smallest (bottom).
		CheckbookLightning32pt7b,
		CheckbookLightning30pt7b,
		CheckbookLightning28pt7b,
		CheckbookLightning26pt7b,
		CheckbookLightning24pt7b,
		CheckbookLightning20pt7b
	};

	struct BoundingBox {
		int16_t x = 0;
		int16_t y = 0;
		uint16_t w = 0;
		uint16_t h = 0;
	};

	BoundingBox amountTextBBox;

	std::string getAmountFiatCurrencyString(const float &amount) {
		return util::floatToStringWithPrecision(amount, config::getUnsignedInt("fiatPrecision")) + " " + config::getString("fiatCurrency");
	}

	BoundingBox calculateTextDimensions(const std::string &t_text, const GFXfont font) {
		BoundingBox bbox;
		const char* text = t_text.c_str();
		tft.setTextSize(1);
		tft.setFreeFont(&font);
		const uint16_t textWidth = tft.textWidth(text);
		const uint16_t textHeight = tft.fontHeight();
		bbox.w = textWidth;
		bbox.h = textHeight;
		return bbox;
	}

	GFXfont getBestFitFont(const std::string &text, const FontList fonts, uint16_t max_w = 0, uint16_t max_h = 0) {
		if (max_w == 0) {
			max_w = tft.width();
		}
		if (max_h == 0) {
			max_h = tft.height();
		}
		for (uint8_t index = 0; index < fonts.size(); index++) {
			const GFXfont font = fonts.at(index);
			const BoundingBox bbox = calculateTextDimensions(text, font);
			if (bbox.w <= max_w && bbox.h <= max_h) {
				// Best fit font found.
				logger::write("\"" + text + "\" best fit font index " + std::to_string(index));
				return font;
			}
		}
		// Default to last font in list - should be smallest.
		return fonts.back();
	}

	// Possible values of alignment:
	// 	TL_DATUM = Top left
	// 	TC_DATUM = Top centre
	// 	TR_DATUM = Top right
	// 	ML_DATUM = Middle left
	// 	MC_DATUM = Middle centre
	// 	MR_DATUM = Middle right
	// 	BL_DATUM = Bottom left
	// 	BC_DATUM = Bottom centre
	// 	BR_DATUM = Bottom right
	BoundingBox renderText(
		const std::string &t_text,
		const GFXfont font,
		const uint16_t &color,
		const int16_t x,
		const int16_t y,
		const uint8_t &alignment = MC_DATUM
	) {
		const char* text = t_text.c_str();
		tft.setTextColor(color);
		tft.setTextSize(1);
		tft.setFreeFont(&font);
		BoundingBox bbox = calculateTextDimensions(text, font);
		int16_t cursor_x = x;
		int16_t cursor_y = y;
		tft.setTextDatum(alignment);
		tft.drawString(text, cursor_x, cursor_y);
		bbox.x = cursor_x;
		bbox.y = cursor_y;
		if (alignment == TC_DATUM || alignment == MC_DATUM || alignment == BC_DATUM) {
			bbox.x -= bbox.w / 2;
		} else if (alignment == TR_DATUM || alignment == MR_DATUM || alignment == BR_DATUM) {
			bbox.x += bbox.w;
		} else if (alignment == TL_DATUM || alignment == ML_DATUM || alignment == BL_DATUM) {
			bbox.x -= bbox.w;
		}
		return bbox;
	}

	BoundingBox renderQRCode(
		const std::string &t_data,
		const int16_t x,
		const int16_t y,
		const uint16_t &max_w,
		const uint16_t &max_h,
		const bool &center = true
	) {
		BoundingBox bbox;
		try {
			const char* data = t_data.c_str();
			uint8_t version = 1;
			uint8_t scale = 1;
			while (version <= 40) {
				const uint16_t bufferSize = qrcode_getBufferSize(version);
				QRCode qrcode;
				uint8_t qrcodeData[bufferSize];
				const int8_t result = qrcode_initText(&qrcode, qrcodeData, version, ECC_LOW, data);
				if (result == 0) {
					// QR encoding successful.
					scale = std::min(std::floor(max_w / qrcode.size), std::floor(max_h / qrcode.size));
					const uint16_t w = qrcode.size * scale;
					const uint16_t h = w;
					int16_t box_x = x;
					int16_t box_y = y;
					if (center) {
						box_x -= (w / 2);
						box_y -= (h / 2);
					}
					tft.fillRect(box_x, box_y, w, h, textColor);
					for (uint8_t y = 0; y < qrcode.size; y++) {
						for (uint8_t x = 0; x < qrcode.size; x++) {
							const auto color = qrcode_getModule(&qrcode, x, y) ? bgColor : textColor;
							tft.fillRect(box_x + scale*x, box_y + scale*y, scale, scale, color);
						}
					}
					bbox.x = box_x;
					bbox.y = box_y;
					bbox.w = w;
					bbox.h = h;
					break;
				} else if (result == -2) {
					// Data was too long for the QR code version.
					version++;
				} else if (result == -1) {
					throw std::runtime_error("Unable to detect mode");
				} else {
					throw std::runtime_error("Unknown failure case");
				}
			}
			// Draw a border around the QR code - to improve readability.
			const uint8_t margin_x = std::min(scale, (uint8_t)std::floor((tft.width() - bbox.w) / 2));
			const uint8_t margin_y = std::min(scale, (uint8_t)std::floor((tft.height() - bbox.h) / 2));
			const uint16_t border_x = bbox.x - margin_x;
			const uint16_t border_y = bbox.y - margin_y;
			tft.fillRect(border_x, border_y, margin_x, bbox.h + (margin_y * 2), textColor);// left
			tft.fillRect(bbox.x + bbox.w, border_y, margin_x, bbox.h + (margin_y * 2), textColor);// right
			tft.fillRect(bbox.x, border_y, bbox.w, margin_y, textColor);// top
			tft.fillRect(bbox.x, bbox.y + bbox.h, bbox.w, margin_y, textColor);// bottom
		} catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
			logger::write("Error while rendering QR code: " + std::string(e.what()), "error");
		}
		return bbox;
	}

	void clearScreen() {
		tft.fillScreen(bgColor);
		if (amountTextBBox.w > 0) {
			amountTextBBox.x = 0;
			amountTextBBox.y = 0;
			amountTextBBox.w = 0;
			amountTextBBox.h = 0;
		}
	}
}

namespace screen_tft {

	void init() {
		logger::write("Initializing TFT...");
		tft.begin();
		tft.setRotation(config::getUnsignedShort("tftRotation"));
		logger::write("TFT display width = " + std::to_string(tft.width()), "debug");
		logger::write("TFT display height = " + std::to_string(tft.height()), "debug");
		center_x = tft.width() / 2;
		center_y = tft.height() / 2;
	}

	void showSplashScreen() {
		clearScreen();
		BoundingBox logoBBox;
		{
			const std::string text = "BLESKOMAT";
			const GFXfont font = getBestFitFont(text, brandFonts);
			logoBBox = renderText(text, font, textColor, center_x, center_y - margin);
		}
		{
			const std::string text = i18n::t("press button to begin");
			const uint16_t max_w = ((tft.width() * 9) / 10) - (margin * 2);
			const GFXfont font = getBestFitFont(text, monospaceFonts, max_w, logoBBox.h / 2);
			const int16_t text_y = tft.height() - margin;
			renderText(text, font, textColor, center_x, text_y, BC_DATUM);
		}
	}

	void showInsertFiatScreen(const float &amount) {
		bool renderInstructionText = true;
		if (amountTextBBox.w > 0) {
			// Clear only the text by drawing over it.
			tft.fillRect(
				amountTextBBox.x,
				amountTextBBox.y,
				amountTextBBox.w,
				amountTextBBox.h,
				bgColor
			);
			renderInstructionText = false;
		} else {
			// Clear the whole screen.
			clearScreen();
		}
		{
			const std::string text = getAmountFiatCurrencyString(amount);
			const uint16_t max_w = ((tft.width() * 8) / 10) - (margin * 2);
			const GFXfont font = getBestFitFont(text, monospaceFonts, max_w);
			amountTextBBox = renderText(text, font, textColor, center_x, margin * 8, TC_DATUM);
		}
		if (renderInstructionText) {
			{
				const std::string text = i18n::t("insert coins");
				const uint16_t max_w = ((tft.width() * 8) / 10) - (margin * 2);
				const GFXfont font = getBestFitFont(text, monospaceFonts, max_w, (amountTextBBox.h * 6) / 10);
				const int16_t text_y = amountTextBBox.y + amountTextBBox.h + (margin * 4);
				renderText(text, font, textColor, center_x, text_y, TC_DATUM);
			}
			{
				const std::string text = i18n::t("press button when done");
				const uint16_t max_w = ((tft.width() * 9) / 10) - (margin * 2);
				const GFXfont font = getBestFitFont(text, monospaceFonts, max_w, amountTextBBox.h / 2);
				const int16_t text_y = tft.height() - margin;
				renderText(text, font, textColor, center_x, text_y, BC_DATUM);
			}
		}
	}

	void showTradeCompleteScreen(const float &amount, const std::string &qrcodeData) {
		clearScreen();
		renderQRCode(qrcodeData, center_x, center_y, tft.width(), tft.height());
	}
}
